#pragma once

class AppD3DX12
{
private:
	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\�����~�N.pmd";
	std::string strMotionPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\Motion\\squat2.vmd";
	ComPtr<ID3D12Device> _dev = nullptr;
	ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
	ComPtr<IDXGISwapChain4> _swapChain = nullptr;
	ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
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
	BufferHeapCreator* bufferHeapCreator = nullptr;
	TextureTransporter* textureTransporter = nullptr;
	MappingExecuter* mappingExecuter = nullptr;
	ViewCreator* viewCreator = nullptr;

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
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	VMDMotionInfo* vmdMotionInfo = nullptr;
	PMDActor* pmdActor = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;	
	TextureLoader* textureLoader = nullptr;

	std::vector<DirectX::XMMATRIX>* boneMatrices = nullptr;
	std::map<std::string, BoneNode> bNodeTable;
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

public:
	///Application�̃V���O���g���C���X�^���X�𓾂�
	static AppD3DX12& Instance();

	// �`��̈�Ȃǂ̏�����
	bool PrepareRendering();

	///�p�C�v���C��������
	bool PipelineInit();

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
