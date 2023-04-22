#include <DirectXTex.h>
#pragma comment (lib, "DirectXTex.lib")
#include <Windows.h>
#include<tchar.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>//シェーダーコンパイルに必要
#pragma comment(lib, "d3dcompiler.lib")

#include <d3dx12.h>
#include <string.h>
#include <map>
#include <sys/stat.h>

using namespace DirectX;
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

//windowがメッセージループ中に取得したメッセージを処理するクラス
LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);//OSへアプリ終了の通知
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapChain = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
ID3D10Blob* _vsBlob = nullptr; // 頂点シェーダーオブジェクト格納用
ID3D10Blob* _psBlob = nullptr; // ピクセルシェーダーオブジェクト格納用
ID3DBlob* _rootSigBlob = nullptr; // ルートシグネチャオブジェクト格納用
ID3DBlob* errorBlob = nullptr; // シェーダー関連エラー格納用

const unsigned int window_width = 720;
const unsigned int window_height = 720;

struct Vertex //?　下のvertices直下に宣言すると、std::copyでエラー発生する
{
	XMFLOAT3 pos; // xyz座標
	XMFLOAT2 uv; // uv座標
};

unsigned short indices[] =
{
	0,1,2,
	2,1,3
};

//PMDヘッダー構造体
struct PMDHeader
{
	float version; // 00 00 80 3F == 1.00
	char model_name[20]; // モデル名
	char comment[256]; // コメント
};

//PMD 頂点構造体
struct PMDVertex
{   //38byte
	float pos[3]; // x, y, z // 座標 12byte
	float normal_vec[3]; // nx, ny, nz // 法線ベクトル 12byte
	float uv[2]; // u, v // UV座標 // MMDは頂点UV 8byte
	unsigned short bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響 4byte
	unsigned char bone_weight; // ボーン1に与える影響度 // min:0 max:100 // ボーン2への影響度は、(100 - bone_weight) 1byte
	unsigned char edge_flag; // 0:通常、1:エッジ無効 // エッジ(輪郭)が有効の場合 1byte
};

//シェーダー側に渡す基本的な行列データ
struct SceneMatrix
{
	XMMATRIX world; // モデル本体の回転・移動行列
	XMMATRIX view; // ビュー行列
	XMMATRIX proj; // プロジェクション行列
	XMFLOAT3 eye; // 視点座標
};

//マテリアル読み込み用の構造体2セット
struct PMDMaterialSet1
{ //46Byte読み込み用
	XMFLOAT3 diffuse;
	float alpha;
	float specularity;
	XMFLOAT3 specular;
	XMFLOAT3 ambient;
	unsigned char toonIdx;
	unsigned char edgeFlg;
};
struct PMDMaterialSet2
{ //24Byte読み込み用
	unsigned int indicesNum;
	char texFilePath[20];
};

//読み込んだマテリアル構造体の出力用に分類
struct MaterialForHlsl // メイン
{
	XMFLOAT3 diffuse;
	float alpha;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
};
struct AdditionalMaterial // サブ
{
	std::string texPath;
	int toonIdx;
	bool edgeFlg;
};
struct Material // 集合体
{
	unsigned int indiceNum;
	MaterialForHlsl material;
	AdditionalMaterial addtional;
};

void EnableDebugLayer() {
	ID3D12Debug* debuglayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debuglayer));

	debuglayer->EnableDebugLayer();
	debuglayer->Release();//有効化後にインターフェイスを解放する
}

//モデルのパスとテクスチャのパスから合成パスを得る
//@param modelPath アプリからみたpmdモデルのパス
//@param texPath pmdモデルからみたテクスチャのパス
//@param return アプリからみたテクスチャのパス
std::string GetTexPathFromModeAndTexlPath
(
	const std::string modelPath,
	const char* texPath
)
{
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1); // 末尾の\も取得するため +1

	return folderPath + texPath;
}
size_t AlignmentSize(size_t size, size_t alignment);
// std::string(マルチバイト文字列)→std::wstring(ワイド文字列)を得る
// @param str マルチバイト文字列
// return 変換されたワイド文字列
std::wstring GetWideStringFromSring(const std::string& str)
{
	//呼び出し一回目(文字列数を得る)
	auto num1 = MultiByteToWideChar // 関数成功でlpWideCharStrが指すバッファに書き込まれたワイド文字の数が返る
	(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), // 変換する文字列へのポインタを指定
		-1, // pMultiByteStr が指す文字列のサイズをバイト単位で渡す。-1でNULL終端と見なされ、長さが自動的に計算される
		nullptr, // 変換後の文字列を受け取るバッファへのポインタを指定
		0 // lpWideCharStrが指すバッファサイズをワイド文字数の単位で指定。 0で必要なバッファのサイズ(ワイド文字数)が返る
	);

	std::wstring wstr; // stringのwchar_t版
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar
	(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0], // wstr[0]からnum1分のワイド文字書き込み
		num1
	);

	assert(num1 == num2);
	return wstr;
}

//ファイルから拡張子を取得する
// @param path 対象パス文字列
// @return 拡張子
std::string GetExtension(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

//テクスチャのパスをセパレータ文字で分離する
// @param path 対象のパス
// @param splitter 文字列
// @return 分離前後の文字列ペア
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter = '*')
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);

	return ret;
}

// テクスチャ用　CPUからのアップロード用バッファ、GPUからの読み取り用バッファ、DirectX::Image生成
// @param metaData ロードしたファイルのTexmetadataオブジェクト
// @param img ロードしたファイルのImageオブジェクト
// @return CPUからのアップロード用バッファ,GPUからの読み取り用バッファ,DirectX::TexMetadata,DirectX::Image
std::tuple<ID3D12Resource*, ID3D12Resource*>  LoadTextureFromFile(TexMetadata* metaData, Image* img, std::string& texPath)
{
	std::map<std::string, std::tuple<ID3D12Resource*, ID3D12Resource*>> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	//メソッド内でimg等動的メモリ確保のポインタを返すことは不可能と理解した
	ID3D12Resource* texUploadBuff = nullptr;//テクスチャCPUアップロード用バッファー

	//テクスチャバッファー用のCPU特化型ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES texUploadHeapProp;
	texUploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	texUploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texUploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texUploadHeapProp.CreationNodeMask = 0; // 単一アダプターのため
	texUploadHeapProp.VisibleNodeMask = 0; // 単一アダプターのため

	//アップロード用
	D3D12_RESOURCE_DESC texUploadResourceDesc = {};
	texUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texUploadResourceDesc.Alignment = 0;
	texUploadResourceDesc.Width = AlignmentSize(img->slicePitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;// *5;
	texUploadResourceDesc.Height = 1;
	texUploadResourceDesc.DepthOrArraySize = 1;
	texUploadResourceDesc.MipLevels = 1;
	texUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	texUploadResourceDesc.SampleDesc.Count = 1;
	texUploadResourceDesc.SampleDesc.Quality = 0;
	texUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//CPUからのアップロード用テクスチャバッファーを作成
	HRESULT result = _dev->CreateCommittedResource
	(&texUploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texUploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadタイプのヒープにおける推奨設定
		nullptr,
		IID_PPV_ARGS(&texUploadBuff)
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}


	ID3D12Resource* texReadBuff = nullptr;//テクスチャGPU読み取り用バッファー

	//テクスチャバッファー用のGPU特化型ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES texReadHeapProp = {};
	texReadHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	texReadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texReadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texReadHeapProp.CreationNodeMask = 0; // 単一アダプターのため
	texReadHeapProp.VisibleNodeMask = 0; // 単一アダプターのため

	//GPU読み取り用
	D3D12_RESOURCE_DESC texReadResourceDesc = {};
	texReadResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData->dimension);
	texReadResourceDesc.Alignment = 0;
	texReadResourceDesc.Width = metaData->width;
	texReadResourceDesc.Height = metaData->height;
	texReadResourceDesc.DepthOrArraySize = metaData->arraySize;
	texReadResourceDesc.MipLevels = metaData->mipLevels;
	texReadResourceDesc.Format = metaData->format;
	texReadResourceDesc.SampleDesc.Count = 1;
	texReadResourceDesc.SampleDesc.Quality = 0;
	texReadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texReadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//GPUからの読み取り用テクスチャバッファーを作成
	result = _dev->CreateCommittedResource
	(&texReadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texReadResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // バッファーがCPUからのリソースコピー先であることを示す
		nullptr,
		IID_PPV_ARGS(&texReadBuff)
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}

	_resourceTable[texPath] = std::forward_as_tuple(texUploadBuff, texReadBuff);
	return std::forward_as_tuple(texUploadBuff, texReadBuff);
}

ID3D12Resource* CreateMappedSphSpaTexResource(TexMetadata* metaData, Image* img, std::string texPath)
{
	// FlyWeight Pattern：同一テクスチャからのリソース生成を防ぎ、既存のリソースを返す
	std::map<std::string, ID3D12Resource*> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProps.VisibleNodeMask = 0;
	heapProps.CreationNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData->dimension);
	resDesc.Alignment = 0;
	resDesc.Width = metaData->width;
	resDesc.Height = metaData->height;
	resDesc.DepthOrArraySize = metaData->arraySize;
	resDesc.MipLevels = metaData->mipLevels;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// B8G8R8X8でsrv生成するとエラー metaData->format;
	//resDesc.Format = metaData->format;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* MappedBuff = nullptr;
	auto result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&MappedBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = MappedBuff->WriteToSubresource
	(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = MappedBuff;
	return MappedBuff;
}

ID3D12Resource* CreateColorTexture(const int param) 
{
	ID3D12Resource* whiteBuff = nullptr;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProps.VisibleNodeMask = 0;
	heapProps.CreationNodeMask = 0;

	D3D12_RESOURCE_DESC whiteResDesc = {};
	whiteResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	whiteResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	whiteResDesc.Width = 4;
	whiteResDesc.Height = 4;
	whiteResDesc.DepthOrArraySize = 1;
	whiteResDesc.SampleDesc.Count = 1;
	whiteResDesc.SampleDesc.Quality = 0;
	whiteResDesc.MipLevels = 1;
	whiteResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	whiteResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&whiteResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), param);

	result = whiteBuff->WriteToSubresource
	(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size() // ソースデータの1つの深度スライスから次のデータまでの距離、つまりサイズ
	);

	return whiteBuff;
}

//トゥーンのためのグラデーションテクスチャ
ID3D12Resource* CreateGrayGradationTexture() {

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//幅
	resDesc.Height = 256;//高さ
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	ID3D12Resource* grayBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&grayBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}

	//上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0x30;
	int i = 0;
	for (; it != data.end(); it += 4) {
		auto col = (0000 << 24) | RGB(c, c, c);
		std::fill(it, it + 3, col);
		i++;
		if (i == 85)
		{
			c = 0xa0;
		}

		if (i == 170)
		{
			c = 0xff;
		}
	}

	result = grayBuff->WriteToSubresource
	(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size()
	);

	return grayBuff;
}

//アライメントにそろえたサイズを返す
//@param size 元のサイズ
//@param alignment アライメントサイズ
//@return アライメントを揃えたサイズ
size_t AlignmentSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

	//PMDヘッダファイルの読み込み
	char signature[3] = {}; // シグネチャ
	PMDHeader pmdHeader = {};
	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd";
	auto fp = fopen(strModelPath.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	//PMD頂点情報の読み込み
	unsigned int vertNum; // 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr size_t pmdvertex_size = 38;
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	//面頂点リスト
	std::vector<unsigned short> indices;
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);

	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	//マテリアル読み込みとシェーダーへの出力準備
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);

	std::vector<PMDMaterialSet1> pmdMat1(materialNum);
	std::vector<PMDMaterialSet2> pmdMat2(materialNum);
	std::vector<Material> materials(materialNum);

	for (int i = 0; i < materialNum; i++)
	{
		fread(&pmdMat1[i], 46, 1, fp);
		fread(&pmdMat2[i], sizeof(PMDMaterialSet2), 1, fp);

	}

	for (int i = 0; i < materialNum; i++)
	{
		materials[i].indiceNum = pmdMat2[i].indicesNum;
		materials[i].material.diffuse = pmdMat1[i].diffuse;
		materials[i].material.alpha = pmdMat1[i].alpha;
		materials[i].material.specular = pmdMat1[i].specular;
		materials[i].material.specularity = pmdMat1[i].specularity;
		materials[i].material.ambient = pmdMat1[i].ambient;
		materials[i].addtional.texPath = pmdMat2[i].texFilePath;
	}

	//ウィンドウクラスの生成と初期化
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)windowProcedure;
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);

	//上記ウィンドウクラスの登録。WINDCLASSEXとして扱われる。
	RegisterClassEx(&w);

	RECT wrc = { 0,0,window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(
		w.lpszClassName,
		_T("DX12テスト"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

	//ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	D3D12_RECT scissorRect = {};
	scissorRect.top = 0; //切り抜き上座標
	scissorRect.left = 0; //切り抜き左座標
	scissorRect.right = scissorRect.left + window_width; //切り抜き右座標
	scissorRect.bottom = scissorRect.top + window_height; //切り抜き下座標

//●パイプライン初期化　処理１～７
// 
//初期化処理１：デバッグレイヤーをオンに
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	//初期化処理２：デバイスの作成 

			//ファクトリーの生成
	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory))))
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory))))
		{
			return -1;
		}
	}

	//グラボが複数挿入されている場合にアダプターを選択するための処理
	//アダプターの列挙用
	std::vector <IDXGIAdapter*> adapters;
	//特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); //アダプターの説明オブジェクト取得

		//探したいアダプターの名前を確認
		std::wstring strDesc = adesc.Description;
		//printf("%ls\n", strDesc.c_str());
		//printf("%x\n", adesc.DeviceId);

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//Direct3D デバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;//生成可能なバージョンが見つかったらループ中断
		}
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//初期化処理３：コマンドキューの記述用意・作成

		//コマンドキュー生成、詳細obj生成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウト無し
	cmdQueueDesc.NodeMask = 0;//アダプターを一つしか使わないときは0でOK
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//コマンドキューの優先度
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//コマンドリストと合わせる

	//コマンドキュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

	//初期化処理４：スワップチェーンの生成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = window_width;
	swapChainDesc.Height = window_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd( //ここで生成
		_cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapChain);

	//初期化処理５：レンダーターゲットビュー(RTV)の記述子ヒープを作成
			//RTV 記述子ヒープ領域の確保
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	//記述子ヒープの生成　ID3D12DescriptorHeap：記述子の連続したコレクション
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	//以下のように記述することでスワップチェーンの持つ情報を新たなDescオブジェクトにコピーできる
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//スワップチェーンの説明
	//result = _swapChain->GetDesc(&swcDesc);//SWCの説明を取得する

//初期化処理６：フレームリソース(各フレームのレンダーターゲットビュー)を作成
	std::vector<ID3D12Resource*> _backBuffers(swapChainDesc.BufferCount);//リソースバッファー
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();//ヒープの先頭を表す CPU 記述子ハンドルを取得

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swapChainDesc.BufferCount; idx++)
	{   //swapEffect DXGI_SWAP_EFFECT_FLIP_DISCARD の場合は最初のバッファーのみアクセス可能
		result = _swapChain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));//SWCにバッファーのIIDとそのIIDポインタを教える(SWCがレンダリング時にアクセスする)

		_dev->CreateRenderTargetView//リソースデータ(_backBuffers)にアクセスするためのレンダーターゲットビューをhandleアドレスに作成
		(
			_backBuffers[idx],//レンダーターゲットを表す ID3D12Resource オブジェクトへのポインター
			&rtvDesc,//レンダー ターゲット ビューを記述する D3D12_RENDER_TARGET_VIEW_DESC 構造体へのポインター。
			handle//新しく作成されたレンダーターゲットビューが存在する宛先を表す CPU 記述子ハンドル(ヒープ上のアドレス)
		);

		//handleｱﾄﾞﾚｽを記述子のアドレスを扱う記述子サイズ分オフセットしていく
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//初期化処理７：コマンドアロケーターを作成
			//コマンドアロケーター生成>>コマンドリスト作成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));


	//●リソース初期化
	// 初期化処理1：ルートシグネチャ設定

		//サンプラー作成
	D3D12_STATIC_SAMPLER_DESC stSamplerDesc[2] = {};
	stSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	stSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].MipLODBias = 0;
	stSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	stSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	stSamplerDesc[0].MinLOD = 0;
	stSamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	stSamplerDesc[0].ShaderRegister = 0; // レジスター：sの番号
	stSamplerDesc[0].RegisterSpace = 0;
	stSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	stSamplerDesc[1] = stSamplerDesc[0];
	stSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // 繰り返さない
	stSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc[1].ShaderRegister = 1;

	//サンプラーのスロット設定
	D3D12_DESCRIPTOR_RANGE descTableRange[3] = {};
	descTableRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 行列用
	descTableRange[0].NumDescriptors = 1; // 使うDHは1つ
	descTableRange[0].BaseShaderRegister = 0; // CBVレジスタ0番
	descTableRange[0].RegisterSpace = 0;
	descTableRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTableRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // マテリアルビュー用
	descTableRange[1].NumDescriptors = 1; // マテリアルCBVで1つ
	descTableRange[1].BaseShaderRegister = 1; // CBVレジスタ1番
	descTableRange[1].RegisterSpace = 0;
	descTableRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTableRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // シェーダーリソースビュー用
	descTableRange[2].NumDescriptors = 4; // テクスチャ(通常とスフィアファイルの2つ
	descTableRange[2].BaseShaderRegister = 0; // SRVレジスタ0番
	descTableRange[2].RegisterSpace = 0;
	descTableRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1; // デプス用
	rootParam[0].DescriptorTable.pDescriptorRanges = descTableRange;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2; // マテリアルとテクスチャで使う
	rootParam[1].DescriptorTable.pDescriptorRanges = &descTableRange[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = rootParam;
	rootSignatureDesc.NumStaticSamplers = 2;
	rootSignatureDesc.pStaticSamplers = stSamplerDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	result = D3D12SerializeRootSignature //シリアル化
	(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&_rootSigBlob,
		&errorBlob
	);

	ID3D12RootSignature* _rootSignature = nullptr;

	result = _dev->CreateRootSignature
	(
		0,
		_rootSigBlob->GetBufferPointer(),
		_rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature)
	);
	_rootSigBlob->Release();

	// 初期化処理2：シェーダーコンパイル

	result = D3DCompileFromFile
	(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_vsBlob
		, &errorBlob
	);

	result = D3DCompileFromFile
	(
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&_psBlob
		, &errorBlob
	);

	//エラーチェック
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
			return 0;
		}
		else
		{
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());

			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());

		}
	}

	// 初期化処理3：頂点入力レイアウトの作成

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		//座標
		{
			"POSITION",
			0, // 同じセマンティクスに対するインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,
			0, // スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 // 一度に描画するインスタンス数
		},

		//法線ベクトル
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//uv
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0, // スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//ボーン番号
		{
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//法線ベクトル
		{
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//法線ベクトル
		{
			"EDGE_FLG",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};


	// 初期化処理4：パイプライン状態オブジェクト(PSO)のDesc記述してオブジェクト作成

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	gpipeLine.pRootSignature = _rootSignature;

	gpipeLine.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeLine.VS.BytecodeLength = _vsBlob->GetBufferSize();

	gpipeLine.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeLine.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState.MultisampleEnable = false;
	gpipeLine.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeLine.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeLine.RasterizerState.DepthClipEnable = true;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetdDesc = {};
	renderTargetdDesc.BlendEnable = false;//ブレンドを有効にするか無効にするか
	renderTargetdDesc.LogicOpEnable = false;//論理操作を有効にするか無効にするか
	renderTargetdDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeLine.BlendState.AlphaToCoverageEnable = false;
	gpipeLine.BlendState.IndependentBlendEnable = false;
	gpipeLine.BlendState.RenderTarget[0] = renderTargetdDesc;
	gpipeLine.InputLayout.pInputElementDescs = inputLayout;

	gpipeLine.InputLayout.NumElements = _countof(inputLayout);

	gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 1;

	gpipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	gpipeLine.SampleDesc.Count = 1; //1サンプル/ピクセル
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = true;
	gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度バッファーに深度値を描き込む
	gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // ソースデータがコピー先データより小さい場合書き込む
	gpipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	ID3D12PipelineState* _pipelineState = nullptr;

	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(&_pipelineState));


	// 初期化処理5：コマンドリスト生成

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	// 初期化処理6：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
	//cmdList->Close();

	// 初期化処理7：各バッファーを作成して頂点情報を読み込み

	//頂点バッファーとインデックスバッファー用のヒーププロパティ設定
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 0;
	heapProps.VisibleNodeMask = 0;

	//深度バッファー用ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//深度バッファー用リソースディスクリプタ
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用
	depthResDesc.SampleDesc.Count = 1; // 1pixce/1つのサンプル
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//クリアバリュー(特定のリソースのクリア操作を最適化するために使用される値)
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f; // 深さ1.0(最大値)でクリア

	ID3D12Resource* vertBuff = nullptr;//CPUとGPUの共有?バッファー領域(リソースとヒープ)
	ID3D12Resource* idxBuff = nullptr;//CPUとGPUの共有?バッファー領域(リソースとヒープ)
	std::vector<ID3D12Resource*> texUploadBuff(materialNum);//テクスチャCPUアップロード用バッファー
	std::vector<ID3D12Resource*> texReadBuff(materialNum);//テクスチャGPU読み取り用バッファー
	std::vector<ID3D12Resource*> sphMappedBuff(materialNum);//sph用バッファー
	std::vector<ID3D12Resource*> spaMappedBuff(materialNum);//spa用バッファー
	std::vector<ID3D12Resource*> toonUploadBuff(materialNum);//トゥーン用アップロードバッファー
	std::vector<ID3D12Resource*> toonReadBuff(materialNum);//トゥーン用リードバッファー
	ID3D12Resource* matrixBuff = nullptr; // 行列用定数バッファー
	ID3D12Resource* materialBuff = nullptr; // マテリアル用定数バッファー
	ID3D12Resource* depthBuff = nullptr; // デプスバッファー

	//頂点バッファーの作成(リソースと暗黙的なヒープの作成) ID3D12Resourceオブジェクトの内部パラメータ設定
	D3D12_RESOURCE_DESC vertresDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	D3D12_RESOURCE_DESC indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices[0]) * indices.size());
	result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの状態。GPUからして読み取り用
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);

	//インデックスバッファーを作成(リソースと暗黙的なヒープの作成)
	result = _dev->CreateCommittedResource
	(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff)
	);

	//デプスバッファーを作成
	result = _dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(&depthBuff)
	);

	//ファイル形式毎のテクスチャロード処理
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};

	std::vector<DirectX::TexMetadata*> metaData(materialNum);
	std::vector<DirectX::Image*> img(materialNum);
	ScratchImage scratchImg = {};
	result = CoInitializeEx(0, COINIT_MULTITHREADED);

	// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	for (int i = 0; i < materials.size(); i++)
	{
		if (strlen(materials[i].addtional.texPath.c_str()) == 0)
		{
			texUploadBuff[i] = nullptr;
			texReadBuff[i] = nullptr;
			continue;
		}

		std::string texFileName = materials[i].addtional.texPath;

		// ファイル名に*を含む場合の処理
		if (std::count(std::begin(texFileName), std::end(texFileName), '*') > 0)
		{
			auto namePair = SplitFileName(texFileName);

			if (GetExtension(namePair.first) == "sph" || GetExtension(namePair.first) == "spa")
			{
				texFileName = namePair.second;
			}

			else
			{
				texFileName = namePair.first;
			}

		}

		// spa,sph拡張子ファイルはslicepitchが大きすぎてオーバーフロー?するため、バッファー作成に失敗する。
		// 更に詳細は不明だがこれによりなぜかvertBuffのマッピングが失敗するようになるため、一時回避する

		auto texFilePath = GetTexPathFromModeAndTexlPath(strModelPath, texFileName.c_str());
		auto wTexPath = GetWideStringFromSring(texFilePath);
		auto extention = GetExtension(texFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "読み込めないテクスチャが存在します" << std::endl;
			return 0;
		}
		metaData[i] = new TexMetadata;
		result = loadLambdaTable[extention](wTexPath, metaData[i], scratchImg);

		if (scratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector の型にconst適用するとコンパイラにより挙動が変化するため禁止
		img[i] = new Image;
		img[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		img[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		img[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		img[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		img[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		img[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// CPU主導でGPUへsphファイルのバッファ生成・サブリソースへコピー
		// 要リファクタリング
		
		if (GetExtension(texFileName) == "sph")
		{
			sphMappedBuff[i] = CreateMappedSphSpaTexResource(metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			spaMappedBuff[i] = nullptr;
		}

		else if (GetExtension(texFileName) == "spa")
		{
			spaMappedBuff[i] = CreateMappedSphSpaTexResource(metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			sphMappedBuff[i] = nullptr;
		}

		else
		{
			std::tie(texUploadBuff[i], texReadBuff[i]) = LoadTextureFromFile(metaData[i], img[i], texFilePath);
			sphMappedBuff[i] = nullptr;
			spaMappedBuff[i] = nullptr;	
		}
	}

	// トゥーン処理
	std::string toonFilePath = "toon\\";
	struct _stat s = {};
	std::vector<DirectX::TexMetadata*> toonMetaData(materialNum);
	std::vector<DirectX::Image*> toonImg(materialNum);
	ScratchImage toonScratchImg = {};

	for (int i = 0; i < materials.size(); i++) 
	{
		//トゥーンリソースの読み込み
		char toonFileName[16];
		sprintf(toonFileName, "toon%02d.bmp", materials[i].addtional.toonIdx + 1);
		toonFilePath += toonFileName;
		toonFilePath = GetTexPathFromModeAndTexlPath(strModelPath, toonFilePath.c_str());

		auto wTexPath = GetWideStringFromSring(toonFilePath);
		auto extention = GetExtension(toonFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "読み込めないテクスチャが存在します" << std::endl;
			return 0;
		}

		toonMetaData[i] = new TexMetadata;
		result = loadLambdaTable[extention](wTexPath, toonMetaData[i], toonScratchImg);

		if (toonScratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector の型にconst適用するとコンパイラにより挙動が変化するため禁止
		toonImg[i] = new Image;
		toonImg[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		toonImg[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		toonImg[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		toonImg[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		toonImg[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		toonImg[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// tooIdx指定(+1)のtoonファイルが存在する場合
		if (_stat(toonFilePath.c_str(), &s) == 0)
		{
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = LoadTextureFromFile(toonMetaData[i], toonImg[i], toonFilePath);
		}

		else
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
	}

	//行列用定数バッファーの生成
	XMMATRIX worldMat = XMMatrixIdentity();
	//auto worldMat = XMMatrixRotationY(15.0f);
	float angle = 0.0f;

	//ビュー行列の生成・乗算
	XMFLOAT3 eye(0, 15, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat = XMMatrixLookAtLH
	(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up)
	);

	//プロジェクション(射影)行列の生成・乗算
	auto projMat = XMMatrixPerspectiveFovLH
	(
		XM_PIDIV2, // 画角90°
		static_cast<float>(window_height) / static_cast<float>(window_width),
		1.0, // ニア―クリップ
		100.0 // ファークリップ
	);

	D3D12_HEAP_PROPERTIES constBuffProp = {};
	constBuffProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC constBuffResdesc = {};
	constBuffResdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);;
	_dev->CreateCommittedResource
	(
		&constBuffProp,
		D3D12_HEAP_FLAG_NONE,
		&constBuffResdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&matrixBuff)
	);

	//マテリアル用定数バッファーの生成
	auto materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;
	D3D12_HEAP_PROPERTIES materialHeapProp = {};
	D3D12_RESOURCE_DESC materialBuffResDesc = {};
	materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	materialBuffResDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum);

	_dev->CreateCommittedResource
	(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialBuffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	//頂点バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的に頂点データをコピーする。
	//CPUは暗黙的なヒープの情報を得られないため、Map関数によりVRAM上のバッファーにアドレスを割り当てた状態で
	//頂点などの情報をVRAMへコピーしている(次の３つはCPUとGPUどちらもアクセス可能なUPLOADタイプなヒープ故マップ可能)、
	//sという理解。Unmapはコメントアウトしても特に影響はないが...
	unsigned char* vertMap = nullptr;

	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	//インデクスバッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	unsigned short* mappedIdx = nullptr;
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//行列用定数バッファーのマッピング
	SceneMatrix* mapMatrix = nullptr;
	result = matrixBuff->Map(0, nullptr, (void**)&mapMatrix);
	mapMatrix->world = worldMat;
	mapMatrix->view = viewMat;
	mapMatrix->proj = projMat;
	mapMatrix->eye = eye;

	//マテリアル用バッファーへのマッピング
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}
	materialBuff->Unmap(0, nullptr);

	// テクスチャアップロード用バッファーの仮想アドレスをポインタにマップ(関連付け)して、
	// 仮想的にインデックスデータをコピーする。
	// テクスチャのアップロード用バッファへのマッピング
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (texUploadBuff[matNum] == nullptr) continue;

		auto srcAddress = img[matNum]->pixels;
		auto rowPitch = AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint8_t* mapforImg = nullptr; // ピクセルデータのシステムメモリバッファーへのポインターがuint8_t(img->pixcel)
		result = texUploadBuff[matNum]->Map(0, nullptr, (void**)&mapforImg);

		// img:元データの初期アドレス(srcAddress)を元ピッチ分オフセットしながら、補正したピッチ個分(rowPitch)のアドレスを
		// mapforImgにその数分(rowPitch)オフセットを繰り返しつつコピーしていく
		//std::copy_n(img->pixels, img->slicePitch, mapforImg);
		for (int i = 0; i < img[matNum]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[matNum]->rowPitch;
			mapforImg += rowPitch;
		}

		texUploadBuff[matNum]->Unmap(0, nullptr);
	}

	// トゥーンテクスチャも同様にマッピング
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (toonUploadBuff[matNum] == nullptr) continue;

		auto toonSrcAddress = toonImg[matNum]->pixels;
		auto toonrowPitch = AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint8_t* toonmapforImg = nullptr;
		result = toonUploadBuff[matNum]->Map(0, nullptr, (void**)&toonmapforImg);

		for (int i = 0; i < toonImg[matNum]->height; ++i)
		{
			std::copy_n(toonSrcAddress, toonrowPitch, toonmapforImg);
			toonSrcAddress += toonImg[matNum]->rowPitch;
			toonmapforImg += toonrowPitch;
		}

		toonUploadBuff[matNum]->Unmap(0, nullptr);
	}

	// テクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> src(materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> dst(materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc(materialNum);

	// テクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (texUploadBuff[matNum] == nullptr || texReadBuff[matNum] == nullptr) continue;

		src[matNum].pResource = texUploadBuff[matNum];
		src[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src[matNum].PlacedFootprint.Offset = 0;
		src[matNum].PlacedFootprint.Footprint.Width = metaData[matNum]->width;
		src[matNum].PlacedFootprint.Footprint.Height = metaData[matNum]->height;
		src[matNum].PlacedFootprint.Footprint.Depth = metaData[matNum]->depth;
		src[matNum].PlacedFootprint.Footprint.RowPitch =
			AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * widthの値は256の倍数であること
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//コピー先設定
		dst[matNum].pResource = texReadBuff[matNum];
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//バリア設定
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = texReadBuff[matNum];
			texBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[matNum]);
			_cmdList->Close();
			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			_cmdQueue->Signal(_fence, ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//キューをクリア
			_cmdList->Reset(_cmdAllocator, nullptr);
		}
	}

	// トゥーンテクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonSrc(materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonDst(materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> toonBarriierDesc(materialNum);
	// トゥーンテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (toonUploadBuff[matNum] == nullptr || toonReadBuff[matNum] == nullptr) continue;

		toonSrc[matNum].pResource = toonUploadBuff[matNum];
		toonSrc[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		toonSrc[matNum].PlacedFootprint.Offset = 0;
		toonSrc[matNum].PlacedFootprint.Footprint.Width = toonMetaData[matNum]->width;
		toonSrc[matNum].PlacedFootprint.Footprint.Height = toonMetaData[matNum]->height;
		toonSrc[matNum].PlacedFootprint.Footprint.Depth = toonMetaData[matNum]->depth;
		toonSrc[matNum].PlacedFootprint.Footprint.RowPitch =
			AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * widthの値は256の倍数であること
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//コピー先設定
		toonDst[matNum].pResource = toonReadBuff[matNum];
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//バリア設定
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = toonReadBuff[matNum];
			toonBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toonBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			toonBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &toonBarriierDesc[matNum]);
			_cmdList->Close();
			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			_cmdQueue->Signal(_fence, ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//キューをクリア
			_cmdList->Reset(_cmdAllocator, nullptr);
		}
	}

	//行列CBV,SRVディスクリプタヒープ作成
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC basicDescHeapDesc = {};
	basicDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	basicDescHeapDesc.NumDescriptors = 1 + materialNum * 5; // 行列cbv,material cbv + テクスチャsrv, sph,spa,toon
	basicDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	basicDescHeapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap
	(
		&basicDescHeapDesc,
		IID_PPV_ARGS(&basicDescHeap)
	);

	//DSVビュー用にディスクリプタヒープ作成
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;

	result = _dev->CreateDescriptorHeap
	(
		&dsvHeapDesc,
		IID_PPV_ARGS(&dsvHeap)
	);

	// 初期化処理8：各ビューを作成

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = vertices.size();//全バイト数
	vbView.StrideInBytes = pmdvertex_size;//1頂点あたりのバイト数

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.SizeInBytes = sizeof(indices[0]) * indices.size();
	ibView.Format = DXGI_FORMAT_R16_UINT;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {}; // 行列用
	cbvDesc.BufferLocation = matrixBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = matrixBuff->GetDesc().Width;

	D3D12_CONSTANT_BUFFER_VIEW_DESC materialCBVDesc = {}; // マテリアル情報、テクスチャ、sph
	materialCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	materialCBVDesc.SizeInBytes = materialBuffSize;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView
	(
		depthBuff,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);
	
	//行列用cbv,マテリアル情報用cbv,テクスチャ用srvを順番に生成
	_dev->CreateConstantBufferView
	(
		&cbvDesc,
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()//basicDescHeapHandle
	);

	auto basicDescHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	basicDescHeapHandle.ptr += inc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	// 白テクスチャバッファ
	ID3D12Resource* whiteBuff = nullptr;
	whiteBuff = CreateColorTexture(0xff);
	// 黒テクスチャバッファ
	ID3D12Resource* BlackBuff = nullptr;
	BlackBuff = CreateColorTexture(0x00);
	// グレーグラデーション
	ID3D12Resource* grayTexBuff = nullptr;
	grayTexBuff = CreateGrayGradationTexture();

	//マテリアル用のcbv,srvを作成
	for (int i = 0; i < materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialCBVDesc.BufferLocation += materialBuffSize;
		
		// テクスチャ
		if (texReadBuff[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = texReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(texReadBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// sphファイル
		if (sphMappedBuff[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = sphMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(sphMappedBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// spaファイル
		if (spaMappedBuff[i] == nullptr)
		{
			srvDesc.Format = BlackBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(BlackBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = spaMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(spaMappedBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// トゥーンテクスチャファイル
		if (toonReadBuff[i] == nullptr)
		{
			srvDesc.Format = grayTexBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(grayTexBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = toonReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(toonReadBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;
	}

	//// 初期化処理9：フェンスの生成
	//	ID3D12Fence* _fence = nullptr;
	//	UINT64 _fenceVal = 0;
	//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// 初期化処理10：イベントハンドルの作成
	// 初期化処理11：GPUの処理完了待ち

		//●描画

	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリ終了時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		//以下は不要。_rootSignatureが_pipelineStateに組み込まれており、SetPipe...でまとめてセットされているから。
		//_cmdList->SetGraphicsRootSignature(_rootSignature);

		_cmdList->SetPipelineState(_pipelineState);
		_cmdList->SetGraphicsRootSignature(_rootSignature);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorRect);

		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//現在のバックバッファをインデックスにて取得

		//リソースバリアの準備
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		//リソースバリア：リソースへの複数のアクセスを同期する必要があることをドライバーに通知
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//ハンドルの初期値アドレスにバッファインデックスを乗算し、各ハンドルの先頭アドレスを計算
		handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // auto rtvhでhandleに上書きでも可
		handle.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvh = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->OMSetRenderTargets(1, &handle, true, &dsvh);//レンダーターゲットと深度ステンシルの CPU 記述子ハンドルを設定
		_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア
		//画面クリア
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);

		//プリミティブ型に関する情報と、入力アセンブラーステージの入力データを記述するデータ順序をバインド
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//頂点バッファーのCPU記述子ハンドルを設定
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		//インデックスバッファーのビューを設定
		_cmdList->IASetIndexBuffer(&ibView);

		//ディスクリプタヒープ設定および
		//ディスクリプタヒープとルートパラメータの関連付け
		//ここでルートシグネチャのテーブルとディスクリプタが関連付く
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable
		(
			0, // バインドのスロット番号
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		);

		//テキストのように同時に二つの同タイプDHをセットすると、グラボによっては挙動が変化する。
		// 二つ目のセットによりNS300/Hではモデルが表示されなくなった。
		//_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		//_cmdList->SetGraphicsRootDescriptorTable
		//(
		//	1, // バインドのスロット番号
		//	basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		//);

		// マテリアルの
		auto materialHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto materialHInc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		materialHandle.ptr += inc;
		unsigned int idxOffset = 0;

		for (auto m : materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
			//インデックス付きインスタンス化されたプリミティブを描画
			_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset, 0, 0);

			materialHandle.ptr += materialHInc;
			idxOffset += m.indiceNum;
		}

		//_cmdList->DrawInstanced(vertNum ,1, 0, 0);

		//ﾊﾞｯｸﾊﾞｯﾌｧ表示前にリソースをCOMMON状態に移行
		//コマンドリストクローズ後は、コマンドリストが特定の呼び出し(Reset())以外は受け付けず、以下3行はエラーになる
		//クローズ後にコマンドキューを実行しているが、ここでリソースの状態が適用される。ここまでにCOMMONから状態を
		//変更させておく必要があるが、実質は●●●から～クローズまでに変更させる必要がある。
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//初期化処理：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
		_cmdList->Close();

		//コマンドキューの実行
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//ID3D12FenceのSignalはCPU側のフェンスで即時実行
		//ID3D12CommandQueueのSignalはGPU側のフェンスで
		//コマンドキューに対する他のすべての操作が完了した後にフェンス更新
		_cmdQueue->Signal(_fence, ++_fenceVal);

		while (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			//イベント発生待ち
			WaitForSingleObject(event, INFINITE);
			//イベントハンドルを閉じる
			CloseHandle(event);
		}

		_cmdAllocator->Reset();//コマンド アロケーターに関連付けられているメモリを再利用する
		_cmdList->Reset(_cmdAllocator, nullptr);//コマンドリストを、新しいコマンドリストが作成されたかのように初期状態にリセット

		//行列情報の更新
		angle += 0.01f;
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;


		//フリップしてレンダリングされたイメージをユーザーに表示
		_swapChain->Present(1, 0);
	}

	UnregisterClass(w.lpszClassName, w.hInstance);
}




