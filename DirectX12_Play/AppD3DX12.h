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
	ComPtr<ID3D10Blob> _vsBlob = nullptr; // ���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3D10Blob> _psBlob = nullptr; // �s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> _rootSigBlob = nullptr; // ���[�g�V�O�l�`���I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> errorBlob = nullptr; // �V�F�[�_�[�֘A�G���[�i�[�p

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

	ComPtr<ID3D12Resource> CreateGrayGradationTexture();

	//window�����b�Z�[�W���[�v���Ɏ擾�������b�Z�[�W����������N���X
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
	///Application�̃V���O���g���C���X�^���X�𓾂�
	static AppD3DX12& Instance();

	///������
	bool Init();

	///���[�v�N��
	void Run();

	///�㏈��
	void Terminate();

	AppD3DX12();

	~AppD3DX12();


};