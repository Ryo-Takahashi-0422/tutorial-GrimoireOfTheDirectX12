#pragma once

class PMDActor
{
private:
	DWORD _startTime; // �A�j���[�V�����J�n���̃~���b
	DWORD elapsedTime; // �o�߃~���b
	unsigned int frameNo; // ���݂̃t���[��No
	std::vector<PMDIK> _ikData; // ik�f�[�^�Q
	PMDMaterialInfo* pmdMaterialInfo;
	std::vector<DirectX::XMMATRIX> boneMatrices;

public:
	// �R�s�[�R���X�g���N�^
	PMDActor(PMDMaterialInfo* _pmdMatInfo);
	unsigned int GetFrameNo();
	void PlayAnimation();
	void MotionUpdate(unsigned int maxFrameNum);
	float GetYFromXOnBezier(float x,const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

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

	// CCD-IK�ɂ��{�[������������ node�\���Froot-interminiate(2�ȏ�)-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCCDIK(const PMDIK& ik);

	// �]���藝IK�ɂ��{�[������������ node�\���Froot-interminiate-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCosineIK(const PMDIK& ik);

	// LOOKAT�s��ɂ��{�[������������ node�\���Froot-target
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveLookAtIK(const PMDIK& ik);
};