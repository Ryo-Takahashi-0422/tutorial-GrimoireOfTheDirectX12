#pragma once

class PMDActor
{
private:
	DWORD _startTime; // �A�j���[�V�����J�n���̃~���b
	DWORD elapsedTime; // �o�߃~���b
	unsigned int frameNo; // ���݂̃t���[��No
	std::vector<PMDIK> ikData; // ik�f�[�^�Q
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	VMDMotionInfo* vmdMotionInfo = nullptr;
	size_t pmdBonesNum;
	std::vector<DirectX::XMMATRIX> boneMatrices;
	std::map<std::string, BoneNode> bNodeTable;
	void RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat);
	unsigned int _duration; // �A�j���[�V�����̍ő�t���[���ԍ�
	std::vector<BoneNode*> boneNodeAddressArray;
	float epsilon;

	// CCD-IK�ɂ��{�[������������ node�\���Froot-interminiate(2�ȏ�)-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCCDIK(const PMDIK& ik);
	std::vector<XMVECTOR> bonePositions; // IK�\���_��ۑ�

	// �]���藝IK�ɂ��{�[������������ node�\���Froot-interminiate-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCosineIK(const PMDIK& ik);
	std::vector<XMVECTOR> positions; // IK�\���_��ۑ�
	std::array<float, 2> edgeLens; // IK�̊e�{�[���Ԃ̋�����ۑ�
	std::vector<uint32_t> kneeIdxs;

	// LOOKAT�s��ɂ��{�[������������ node�\���Froot-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveLookAtIK(const PMDIK& ik);



public:
	//std::vector<DirectX::XMMATRIX> boneMatrices;
	// �R�s�[�R���X�g���N�^
	PMDActor(PMDMaterialInfo* _pmdMatInfo, VMDMotionInfo* _vmdMotionInfo);
	unsigned int GetFrameNo();
	void PlayAnimation();
	void MotionUpdate(unsigned int maxFrameNum);
	float GetYFromXOnBezier(float x,const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);
	std::vector<DirectX::XMMATRIX>* GetMatrices();
	unsigned int GetDuration();


	// �A�j���[�V���������X�V���ă{�[��(�e)�̈ʒu��p�x��ω�������
	void UpdateVMDMotion();

	// �`��O�ɃA�j���[�V�����ɂ��e�{�[���̊p�x�ω����ċA�I�Ɏq�{�[���֓`�B����
	void RecursiveMatrixMultiply(const DirectX::XMMATRIX& mat);

	// z�������̕����Ɍ�������s���Ԃ��֐�
	// @param lookat ���������������x�N�g��
	// @param up ��x�N�g��
	// @param right �E�x�N�g��
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);

	///����̃x�N�g�������̕����Ɍ����邽�߂̍s���Ԃ�
	///@param origin ����̃x�N�g��
	///@param lookat ��������������
	///@param up ��x�N�g��
	///@param right �E�x�N�g��
	///@retval ����̃x�N�g�������̕����Ɍ����邽�߂̍s��
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);
	
	// IK�̓��o�p�^�[�������肷��
	void IKSolve();


};