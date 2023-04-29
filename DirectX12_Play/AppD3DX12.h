#pragma once

class AppD3DX12
{
private:
	ComPtr<ID3D12Device> _dev = nullptr;
	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain4> _swapChain = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // 頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _psBlob = nullptr; // ピクセルシェーダーオブジェクト格納用
	ComPtr<ID3DBlob> _rootSigBlob = nullptr; // ルートシグネチャオブジェクト格納用
	ComPtr<ID3DBlob> errorBlob = nullptr; // シェーダー関連エラー格納用

	const unsigned int window_width = 720;
	const unsigned int window_height = 720;

	struct Vertex //?　下のvertices直下に宣言すると、std::copyでエラー発生する
	{
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv; // uv座標
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

	ComPtr<ID3D12Resource> CreateGrayGradationTexture();

	//windowがメッセージループ中に取得したメッセージを処理するクラス
	//LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void EnableDebugLayer();
	std::string GetTexPathFromModeAndTexlPath(const std::string modelPath,const char* texPath);
	std::wstring GetWideStringFromSring(const std::string& str);
	std::string GetExtension(const std::string& path);
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
	std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>
		LoadTextureFromFile(TexMetadata* metaData, Image* img, std::string& texPath);
	ComPtr<ID3D12Resource> CreateMappedSphSpaTexResource(TexMetadata* metaData, Image* img, std::string texPath);
	ComPtr<ID3D12Resource> CreateColorTexture(const int param);

public:
	///Applicationのシングルトンインスタンスを得る
	static AppD3DX12& Instance();

	///初期化
	bool Init();

	///ループ起動
	void Run();

	///後処理
	void Terminate();

	AppD3DX12();

	~AppD3DX12();


};