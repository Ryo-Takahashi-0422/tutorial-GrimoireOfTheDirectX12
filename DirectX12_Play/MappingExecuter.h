#pragma once

class MappingExecuter
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	// �}�b�s���O��|�C���^�Q
	unsigned char* vertMap = nullptr;
	unsigned short* mappedIdx = nullptr;
	char* mapMaterial = nullptr;
	uint8_t* mapforImg = nullptr;
	float* mappedweight = nullptr;

	HRESULT result;

public:
	MappingExecuter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);

	//���_�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɒ��_�f�[�^���R�s�[����B
	void MappingVertBuff();

	//�C���f�N�X�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	void MappingIndexOfVertexBuff();

	//�}�e���A���p�o�b�t�@�[�ւ̃}�b�s���O
	void MappingMaterialBuff();

	// �e�N�X�`���A�b�v���[�h�p�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	void TransferTexUploadToBuff(std::vector<ComPtr<ID3D12Resource>> uploadBuff, std::vector<DirectX::Image*> img, unsigned int itCount);

	// �K�E�V�A���ڂ����o�b�t�@�[�ւ̃E�F�C�g�l�}�b�s���O
	void MappingGaussianWeight(std::vector<float> weights);


};