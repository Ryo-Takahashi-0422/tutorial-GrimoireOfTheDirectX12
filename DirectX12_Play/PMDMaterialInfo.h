#pragma once
using namespace DirectX;

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
		XMMATRIX shadow; // �e
		XMFLOAT3 eye; // ���_���W
		XMMATRIX bones[256]; // �{�[���s��
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

#pragma pack(1)
	struct PMDBone // �{�[�����
	{
		char boneName[20]; // �{�[����
		unsigned short parentIndex; // �e�{�[���ԍ�(�Ȃ��ꍇ��0xFFFF)
		unsigned short tailIndex; // tail�ʒu�̃{�[���ԍ�(�`�F�[�����[�̏ꍇ��0xFFFF 0 ���⑫2) // �e�F�q��1�F���Ȃ̂ŁA��Ɉʒu���ߗp
		unsigned char type; // �{�[���̎��
		unsigned short ikParentIndex; // IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0)
		XMFLOAT3 headPos; // �{�[���̃w�b�h�̈ʒu
	};
#pragma pack()

	struct BoneNode
	{
		uint32_t boneIdx; // �{�[���C���f�b�N�X
		uint32_t boneType; // �{�[����� 0:��],1:��]���ړ�,2:IK,...7:�s���{�[��
		uint32_t ikParentBone; // IK�e�{�[��
		XMFLOAT3 startPos; // �{�[����_(��]���S)
		//XMFLOAT3 endPos; // �{�[����[�_
		std::vector<BoneNode*> children; // �q�m�[�h
	};

	struct PMDIK // PMD���f����IK���
	{
		uint16_t boneidx; // IK�{�[���ԍ�
		uint16_t targetidx; // IK�^�[�Q�b�g�{�[���ԍ� // IK�{�[�����ŏ��ɐڑ�����{�[��		
		uint16_t iterations; // �ċA���Z�� // IK�l1
		float limit; // ���Z1�񂠂���̐����p�x // IK�l2
		std::vector<uint16_t> nodeIdx; // IK�e�����̃{�[���ԍ�
	};

class PMDMaterialInfo
{
private:
	unsigned short boneNum;
	std::vector<DirectX::XMMATRIX> _boneMatrice;
	std::vector<PMDBone> pmdBones;
	std::map<std::string, BoneNode> _boneNodeTable;
	std::vector<std::string> boneName;
	uint16_t ikNum;
	uint8_t chainLen; // IK�`�F�[���̒���(�Ԃ̃m�[�h��)
	std::vector<PMDIK> pmdIkData; // IK���R���e�i
	std::vector<uint32_t> kneeIdxs;
	std::vector<BoneNode*> _boneNodeAddressArray; // �C���f�b�N�X����m�[�h����������p

public:

	HRESULT ReadPMDHeaderFile(std::string);

	float angle;
	XMMATRIX worldMat;
	SceneMatrix* mapMatrix = nullptr;

	//std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\�����~�N.pmd";
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
	//std::vector<DirectX::XMMATRIX> GetBoneMatrices();
	size_t GetNumberOfBones();
	std::map<std::string, BoneNode> GetBoneNode();
	std::vector<uint32_t> GetKneeIdx();
	std::vector<PMDIK> GetpPMDIKData();
	const std::vector<BoneNode*> GetBoneNodeAddressArray();

	std::vector<std::string> _boneNameArray; // �C���f�b�N�X����{�[��������������p
	
};
