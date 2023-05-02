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
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // ���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _psBlob = nullptr; // �s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> _rootSigBlob = nullptr; // ���[�g�V�O�l�`���I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> errorBlob = nullptr; // �V�F�[�_�[�֘A�G���[�i�[�p
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

	ComPtr<ID3D12Resource> vertBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	ComPtr<ID3D12Resource> idxBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // �s��p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> materialBuff = nullptr; // �}�e���A���p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> depthBuff = nullptr; // �f�v�X�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//�g�D�[���p���[�h�o�b�t�@�[
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

	struct Vertex //?�@����vertices�����ɐ錾����ƁAstd::copy�ŃG���[��������
	{
		XMFLOAT3 pos; // xyz���W
		XMFLOAT2 uv; // uv���W
	};

	//PMD�w�b�_�[�\����
	struct PMDHeader
	{
		float version; // 00 00 80 3F == 1.00
		char model_name[20]; // ���f����
		char comment[256]; // �R�����g
	};

	//PMD ���_�\����
	struct PMDVertex
	{   //38byte
		float pos[3]; // x, y, z // ���W 12byte
		float normal_vec[3]; // nx, ny, nz // �@���x�N�g�� 12byte
		float uv[2]; // u, v // UV���W // MMD�͒��_UV 8byte
		unsigned short bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe�� 4byte
		unsigned char bone_weight; // �{�[��1�ɗ^����e���x // min:0 max:100 // �{�[��2�ւ̉e���x�́A(100 - bone_weight) 1byte
		unsigned char edge_flag; // 0:�ʏ�A1:�G�b�W���� // �G�b�W(�֊s)���L���̏ꍇ 1byte
	};

	//�V�F�[�_�[���ɓn����{�I�ȍs��f�[�^
	struct SceneMatrix
	{
		XMMATRIX world; // ���f���{�̂̉�]�E�ړ��s��
		XMMATRIX view; // �r���[�s��
		XMMATRIX proj; // �v���W�F�N�V�����s��
		XMFLOAT3 eye; // ���_���W
	};

	float angle;
	XMMATRIX worldMat;
	SceneMatrix* mapMatrix = nullptr;

	//�}�e���A���ǂݍ��ݗp�̍\����2�Z�b�g
	struct PMDMaterialSet1
	{ //46Byte�ǂݍ��ݗp
		XMFLOAT3 diffuse;
		float alpha;
		float specularity;
		XMFLOAT3 specular;
		XMFLOAT3 ambient;
		unsigned char toonIdx;
		unsigned char edgeFlg;
	};

	struct PMDMaterialSet2
	{ //24Byte�ǂݍ��ݗp
		unsigned int indicesNum;
		char texFilePath[20];
	};

	//�ǂݍ��񂾃}�e���A���\���̂̏o�͗p�ɕ���
	struct MaterialForHlsl // ���C��
	{
		XMFLOAT3 diffuse;
		float alpha;
		XMFLOAT3 specular;
		float specularity;
		XMFLOAT3 ambient;
	};

	struct AdditionalMaterial // �T�u
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	struct Material // �W����
	{
		unsigned int indiceNum;
		MaterialForHlsl material;
		AdditionalMaterial addtional;
	};

	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\�����~�N.pmd";
	char signature[3] = {}; // �V�O�l�`��
	PMDHeader pmdHeader = {};
	unsigned int vertNum; // ���_��
	size_t pmdvertex_size;
	std::vector<unsigned char> vertices{};
	std::vector<unsigned short> indices{};
	unsigned int indicesNum;
	unsigned int materialNum;
	std::vector<PMDMaterialSet1> pmdMat1;
	std::vector<PMDMaterialSet2> pmdMat2;
	std::vector<Material> materials;

	//window�����b�Z�[�W���[�v���Ɏ擾�������b�Z�[�W����������N���X
	//LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// �V���O���g���Ȃ̂ŃR���X�g���N�^�A�R�s�[�R���X�g���N�^�A������Z�q��private�ɂ���
	// �R���X�g���N�^
	AppD3DX12() {};
	// �R�s�[�R���X�g���N�^
	AppD3DX12(const AppD3DX12& x) { };
	// ������Z�q
	AppD3DX12& operator=(const AppD3DX12&) { return *this; };

	// PMD�t�@�C���̓ǂݍ���
	HRESULT ReadPMDHeaderFile();

	// �����_�����O�E�B���h�E�ݒ�
	void CreateAppWindow();

	// �r���[�|�[�g�ƃV�U�[�̈�̐ݒ�
	void SetViewportAndRect();

	/// <summary>
	/// �e��f�o�C�X�̍쐬 
	/// </summary>
	/// <returns></returns>
	HRESULT D3DX12DeviceInit();

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