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
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // ���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _psBlob = nullptr; // �s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _vsMBlob = nullptr; // ����߽�p���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _psMBlob = nullptr; // ����߽�p���_�s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _vsBackbufferBlob = nullptr; // �\���p���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _psBackbufferBlob = nullptr; // �\���p���_�s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers; // �ܯ�������ޯ��ޯ̧� D3D12_RESOURCE_STATE_COMMON�ɐݒ肷�郋�[���B
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

	// �V���O���g���Ȃ̂ŃR���X�g���N�^�A�R�s�[�R���X�g���N�^�A������Z�q��private�ɂ���
	// �R���X�g���N�^
	AppD3DX12() {};

	// �R�s�[�R���X�g���N�^
	AppD3DX12(const AppD3DX12& x) { };

	// ������Z�q
	AppD3DX12& operator=(const AppD3DX12&) { return *this; };

	// PMD�t�@�C���̓ǂݍ���
	HRESULT ReadPMDHeaderFile();

	/// <summary>
	/// �e��f�o�C�X�̍쐬 
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
	unsigned int _duration; // �A�j���[�V�����̍ő�t���[���ԍ�

	void RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat);
	void UpdateVMDMotion(std::map<std::string, BoneNode> bNodeTable, 
		std::unordered_map<std::string, std::vector<KeyFrame>> motionData);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// ����߽�֘A
	PeraGraphicsPipelineSetting* peraGPLSetting = nullptr;
	PeraLayout* peraLayout = nullptr;
	PeraPolygon* peraPolygon = nullptr;
	PeraSetRootSignature* peraSetRootSignature = nullptr;
	PeraShaderCompile* peraShaderCompile = nullptr;
	PeraGraphicsPipelineSetting* bufferGPLSetting = nullptr;
	PeraSetRootSignature* bufferSetRootSignature = nullptr;

	BufferShaderCompile* bufferShaderCompile = nullptr;

	// ���C�g�}�b�v�֘A
	LightMapGraphicsPipelineSetting* lightMapGPLSetting = nullptr;
	SetRootSignature* lightMapRootSignature = nullptr;
	LightMapShaderCompile* lightMapShaderCompile = nullptr;
	ComPtr<ID3D10Blob> _lightMapVSBlob = nullptr; // ���C�g�}�b�v�p���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _lightMapPSBlob = nullptr; // ���C�g�}�b�v�p�s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	XMFLOAT4 _planeNormalVec;
	XMFLOAT3 lightVec;
	
	// bloom	
	PeraGraphicsPipelineSetting* bloomGPLSetting = nullptr;
	PeraSetRootSignature* bloomRootSignature = nullptr;
	BloomShaderCompile* bloomShaderCompile = nullptr;
	ComPtr<ID3D10Blob> _bloomVSBlob = nullptr; // bloom�p���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _bloomPSBlob = nullptr; // bloom�p�s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	
	// AO
	AOShaderCompile* aoShaderCompile = nullptr;
	AOGraphicsPipelineSetting* aoGPLSetting = nullptr;
	SetRootSignature* aoRootSignature = nullptr;
	ComPtr<ID3D10Blob> _aoVSBlob = nullptr; // AO�p���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _aoPSBlob = nullptr; // AO�p�s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p

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
	GraphicsMemory* _gmemory = nullptr; // ���̨�����ص�޼ު��
	SpriteFont* _spriteFont = nullptr; // �t�H���g�\���p�I�u�W�F�N�g
	SpriteBatch* _spriteBatch = nullptr; // �X�v���C�g�\���p�I�u�W�F�N�g
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
	///Application�̃V���O���g���C���X�^���X�𓾂�
	static AppD3DX12& Instance();

	// �`��̈�Ȃǂ̏�����
	bool PrepareRendering();

	// �p�C�v���C��������
	bool PipelineInit();

	// effekseer initialize
	void EffekseerInit();

	/// <summary>
	/// �e�탊�\�[�X�̏�����
	/// </summary>
	/// <returns></returns>
	bool ResourceInit();

	///���[�v�N��
	void Run();

	///�㏈��
	void Terminate();

	//�f�X�g���N�^
	~AppD3DX12();

};
