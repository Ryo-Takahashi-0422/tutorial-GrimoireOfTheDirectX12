#pragma once

class AppD3DX12
{
private:

	std::array<std::string, 3> strModelPath;
	int strModelNum = 0;
	std::string strMotionPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\Motion\\squat2.vmd";
	ComPtr<ID3D12Device> _dev = nullptr;
	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain4> _swapChain = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocator4Imgui = nullptr;
	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	ComPtr<ID3D12GraphicsCommandList> _cmdList4Imgui = nullptr;
	ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // 頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _psBlob = nullptr; // ピクセルシェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _vsMBlob = nullptr; // ﾏﾙﾁﾊﾟｽ用頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _psMBlob = nullptr; // ﾏﾙﾁﾊﾟｽ用頂点ピクセルシェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _vsBackbufferBlob = nullptr; // 表示用頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _psBackbufferBlob = nullptr; // 表示用頂点ピクセルシェーダーオブジェクト格納用
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers; // ｽﾜｯﾌﾟﾁｪｰﾝﾊﾞｯｸﾊﾞｯﾌｧｰ D3D12_RESOURCE_STATE_COMMONに設定するルール。
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	HRESULT result;

	ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	GraphicsPipelineSetting* gPLSetting = nullptr;
	std::vector<BufferHeapCreator*> bufferHeapCreator;
	std::vector<TextureTransporter*> textureTransporter;
	std::vector<MappingExecuter*> mappingExecuter;
	std::vector<ViewCreator*> viewCreator;

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

	SetRootSignature* setRootSignature = nullptr;
	SettingShaderCompile* settingShaderCompile = nullptr;
	VertexInputLayout* vertexInputLayout = nullptr;
	std::vector<PMDMaterialInfo*> pmdMaterialInfo;
	std::vector<VMDMotionInfo*> vmdMotionInfo;
	std::vector<PMDActor*> pmdActor;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;	
	TextureLoader* textureLoader = nullptr;

	std::map<int, std::vector<DirectX::XMMATRIX>*> boneMatrices;
	std::map<int, std::map<std::string, BoneNode>> bNodeTable;
	unsigned int _duration; // アニメーションの最大フレーム番号

	void RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat);
	void UpdateVMDMotion(std::map<std::string, BoneNode> bNodeTable, 
		std::unordered_map<std::string, std::vector<KeyFrame>> motionData);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// ﾏﾙﾁﾊﾟｽ関連
	PeraGraphicsPipelineSetting* peraGPLSetting = nullptr;
	PeraLayout* peraLayout = nullptr;
	PeraPolygon* peraPolygon = nullptr;
	PeraSetRootSignature* peraSetRootSignature = nullptr;
	PeraShaderCompile* peraShaderCompile = nullptr;
	PeraGraphicsPipelineSetting* bufferGPLSetting = nullptr;
	PeraSetRootSignature* bufferSetRootSignature = nullptr;

	BufferShaderCompile* bufferShaderCompile = nullptr;

	// ライトマップ関連
	LightMapGraphicsPipelineSetting* lightMapGPLSetting = nullptr;
	SetRootSignature* lightMapRootSignature = nullptr;
	LightMapShaderCompile* lightMapShaderCompile = nullptr;
	ComPtr<ID3D10Blob> _lightMapVSBlob = nullptr; // ライトマップ用頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _lightMapPSBlob = nullptr; // ライトマップ用ピクセルシェーダーオブジェクト格納用
	XMFLOAT4 _planeNormalVec;
	XMFLOAT3 lightVec;
	
	// bloom	
	PeraGraphicsPipelineSetting* bloomGPLSetting = nullptr;
	PeraSetRootSignature* bloomRootSignature = nullptr;
	BloomShaderCompile* bloomShaderCompile = nullptr;
	ComPtr<ID3D10Blob> _bloomVSBlob = nullptr; // bloom用頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _bloomPSBlob = nullptr; // bloom用ピクセルシェーダーオブジェクト格納用
	
	// AO
	AOShaderCompile* aoShaderCompile = nullptr;
	AOGraphicsPipelineSetting* aoGPLSetting = nullptr;
	SetRootSignature* aoRootSignature = nullptr;
	ComPtr<ID3D10Blob> _aoVSBlob = nullptr; // AO用頂点シェーダーオブジェクト格納用
	ComPtr<ID3D10Blob> _aoPSBlob = nullptr; // AO用ピクセルシェーダーオブジェクト格納用

	// Imgui
	SettingImgui* settingImgui = nullptr;
	void SetFov();
	float SetBackGroundColor(int rgbaNum);
	void SetSelfShadowLight(int modelNum);
	void SetSelfShadowSwitch(int modelNum);
	void SetBloomSwitch(int modelNum);
	void SetFoVSwitch();
	void SetSSAOSwitch();
	void SetBloomColor();

	DirectX::XMVECTOR light;
	DirectX::XMVECTOR eyePos;
	DirectX::XMVECTOR targetPos;
	DirectX::XMVECTOR upVec;

	// Effekseer
	EffekseerRenderer::RendererRef _efkRenderer = nullptr; // effect renderer
	Effekseer::ManagerRef _efkManager = nullptr; // effect manager
	Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> _efkMemoryPool = nullptr; // memory pool
	Effekseer::RefPtr<EffekseerRenderer::CommandList> _efkCmdList = nullptr; // for DirectX12, Vulkan
	Effekseer::EffectRef _effect = nullptr; // entity of effect(effect file)
	Effekseer::Handle _efkHandle; // effect handle(exceuted effect address)
	Effekseer::Matrix44 fkViewMat;
	Effekseer::Matrix44 fkProjMat;
	void DrawEffect();

	// DirectXTK
	GraphicsMemory* _gmemory = nullptr; // ｸﾞﾗﾌｨｯｸｽﾒﾓﾘｵﾌﾞｼﾞｪｸﾄ
	SpriteFont* _spriteFont = nullptr; // フォント表示用オブジェクト
	SpriteBatch* _spriteBatch = nullptr; // スプライト表示用オブジェクト
	void DirectXTKInit();
	void DrawSpriteFont();

	// draw method
	void DrawLightMap(unsigned int modelNum, UINT buffSize); // draw light map
	void DrawModel(unsigned int modelNum, UINT buffSize); // draw pmd model
	void DrawShrinkTextureForBlur(unsigned int modelNum, UINT buffSize); // draw blur texture
	void DrawPeraPolygon(unsigned int modelNum); // draw background polygon	
	void DrawAmbientOcclusion(unsigned int modelNum, UINT buffSize); // draw ambient occlusion
	void DrawBackBuffer(UINT buffSize); // draw back buffers
	void DrawModel4AO(unsigned int modelNum, UINT buffSize);

	// Matrix
	XMMATRIX projMat;

public:
	///Applicationのシングルトンインスタンスを得る
	static AppD3DX12& Instance();

	// 描画領域などの初期化
	bool PrepareRendering();

	// パイプライン初期化
	bool PipelineInit();

	// effekseer initialize
	void EffekseerInit();

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
