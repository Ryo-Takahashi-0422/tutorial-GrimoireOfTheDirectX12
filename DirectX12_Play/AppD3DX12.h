#pragma once

class AppD3DX12
{
private:
	WNDCLASSEX w;
	HWND hwnd;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
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
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal;
	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	HRESULT result;
	ComPtr<ID3D12InfoQueue> infoQueue = nullptr;

	ComPtr<ID3D12PipelineState> _pipelineState = nullptr;
	ComPtr<ID3D12RootSignature> _rootSignature = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	D3D12_INDEX_BUFFER_VIEW ibView;
	ComPtr<ID3D12DescriptorHeap> basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC basicDescHeapDesc;

	ComPtr<ID3D12Resource> vertBuff = nullptr;//CPUとGPUの共有?バッファー領域(リソースとヒープ)
	ComPtr<ID3D12Resource> idxBuff = nullptr;//CPUとGPUの共有?バッファー領域(リソースとヒープ)
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // 行列用定数バッファー
	ComPtr<ID3D12Resource> materialBuff = nullptr; // マテリアル用定数バッファー
	ComPtr<ID3D12Resource> depthBuff = nullptr; // デプスバッファー
	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//テクスチャCPUアップロード用バッファー
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//テクスチャGPU読み取り用バッファー
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph用バッファー
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa用バッファー
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//トゥーン用アップロードバッファー
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//トゥーン用リードバッファー
	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	std::vector<DirectX::TexMetadata*> metaData;
	std::vector<DirectX::Image*> img;
	std::vector<DirectX::TexMetadata*> toonMetaData;
	std::vector<DirectX::Image*> toonImg;

	//std::unique_ptr<char> vertMap = nullptr;
	//std::weak_ptr<char> vertMap = nullptr;
	//ComPtr<unsigned char> vertMap = nullptr;
	unsigned char* vertMap = nullptr;
	unsigned short* mappedIdx = nullptr;
	char* mapMaterial = nullptr;

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

	float angle;
	XMMATRIX worldMat;
	SceneMatrix* mapMatrix = nullptr;

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

	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd";
	char signature[3] = {}; // シグネチャ
	PMDHeader pmdHeader = {};
	unsigned int vertNum; // 頂点数
	size_t pmdvertex_size;
	std::vector<unsigned char> vertices{};
	std::vector<unsigned short> indices{};
	unsigned int indicesNum;
	unsigned int materialNum;
	std::vector<PMDMaterialSet1> pmdMat1;
	std::vector<PMDMaterialSet2> pmdMat2;
	std::vector<Material> materials;

	//windowがメッセージループ中に取得したメッセージを処理するクラス
	//LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// シングルトンなのでコンストラクタ、コピーコンストラクタ、代入演算子はprivateにする
	// コンストラクタ
	AppD3DX12() {};
	// コピーコンストラクタ
	AppD3DX12(const AppD3DX12& x) { };
	// 代入演算子
	AppD3DX12& operator=(const AppD3DX12&) { return *this; };

	// PMDファイルの読み込み
	HRESULT ReadPMDHeaderFile();

	// レンダリングウィンドウ設定
	void CreateAppWindow();

	// ビューポートとシザー領域の設定
	void SetViewportAndRect();

	/// <summary>
	/// 各種デバイスの作成 
	/// </summary>
	/// <returns></returns>
	HRESULT D3DX12DeviceInit();

public:
	///Applicationのシングルトンインスタンスを得る
	static AppD3DX12& Instance();

	// 描画領域などの初期化
	bool PrepareRendering();

	///パイプライン初期化
	bool PipelineInit();

	/// <summary>
	/// 各種リソースの初期化
	/// </summary>
	/// <returns></returns>
	bool ResourceInit();

	///ループ起動
	void Run();

	///後処理
	void Terminate();

	//デストラクタ
	~AppD3DX12();
};