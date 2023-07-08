#pragma once

class AppD3DX12
{
private:
	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd";
	std::string strMotionPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\Motion\\squat2.vmd";
	ComPtr<ID3D12Device> _dev = nullptr;
	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain4> _swapChain = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // 頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _psBlob = nullptr; // ピクセルシェーダーオブジェクト格納用
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	HRESULT result;
	ComPtr<ID3D12InfoQueue> infoQueue = nullptr;

	ComPtr<ID3D12PipelineState> _pipelineState = nullptr;
	GraphicsPipelineSetting* gPLSetting = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;
	TextureTransporter* textureTransporter = nullptr;
	MappingExecuter* mappingExecuter = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	std::vector<DirectX::TexMetadata*> metaData;
	std::vector<DirectX::Image*> img;
	std::vector<DirectX::TexMetadata*> toonMetaData;
	std::vector<DirectX::Image*> toonImg;

	// シングルトンなのでコンストラクタ、コピーコンストラクタ、代入演算子はprivateにする
	// コンストラクタ
	AppD3DX12() {};

	// コピーコンストラクタ
	AppD3DX12(const AppD3DX12& x) { };

	// 代入演算子
	AppD3DX12& operator=(const AppD3DX12&) { return *this; };

	// PMDファイルの読み込み
	HRESULT ReadPMDHeaderFile();

	/// <summary>
	/// 各種デバイスの作成 
	/// </summary>
	/// <returns></returns>
	HRESULT D3DX12DeviceInit();

	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	//ComPtr<PMDMaterialInfo> pmdMaterialInfo;
	VMDMotionInfo* vmdMotionInfo = nullptr;
	PMDActor* pmdActor = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;	
	SetRootSignature* setRootSignature = nullptr;
	TextureLoader* textureLoader = nullptr;

	std::vector<DirectX::XMMATRIX>* boneMatrices = nullptr;
	std::map<std::string, BoneNode> bNodeTable;
	unsigned int _duration; // アニメーションの最大フレーム番号

	void RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat);
	void UpdateVMDMotion(std::map<std::string, BoneNode> bNodeTable, 
		std::unordered_map<std::string, std::vector<KeyFrame>> motionData);

	

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
