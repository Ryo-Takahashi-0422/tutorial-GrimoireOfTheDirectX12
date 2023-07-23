#pragma once

namespace Utility 
{
	//�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
	//@param size ���̃T�C�Y
	//@param alignment �A���C�����g�T�C�Y
	//@return �A���C�����g�𑵂����T�C�Y
	size_t AlignmentSize(size_t size, size_t alignment);

	void EnableDebugLayer();

	//���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	//@param modelPath �A�v������݂�pmd���f���̃p�X
	//@param texPath pmd���f������݂��e�N�X�`���̃p�X
	//@param return �A�v������݂��e�N�X�`���̃p�X
	std::string GetTexPathFromModeAndTexlPath(const std::string modelPath, const char* texPath);

	// std::string(�}���`�o�C�g������)��std::wstring(���C�h������)�𓾂�
	// @param str �}���`�o�C�g������
	// return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromSring(const std::string& str);

	//�t�@�C������g���q���擾����
	// @param path �Ώۃp�X������
	// @return �g���q
	std::string GetExtension(const std::string& path);

	//�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	// @param path �Ώۂ̃p�X
	// @param splitter ������
	// @return �����O��̕�����y�A
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

	// �K�E�V�A���ڂ����̉������p�E�F�C�g���v�Z
	// @param count �v�Z��
	// @param disp ���U
	// @return �E�F�C�g
	std::vector<float> GetGaussianWeight(size_t count, float disp);
};