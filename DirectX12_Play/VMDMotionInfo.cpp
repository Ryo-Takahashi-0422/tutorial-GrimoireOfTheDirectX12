#include <stdafx.h>

HRESULT VMDMotionInfo::ReadVMDHeaderFile(std::string strMotionPath)
{
	// VMD�w�b�_�t�@�C���̓ǂݍ���
	auto fp = fopen(strMotionPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	// �ŏ���50byte�͕s�v�Ȃ̂Ŕ�΂�
	fseek(fp, 50, SEEK_SET);

	// �f�[�^���ǂݍ���
	motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);
	vmdMotions.resize(motionDataNum);

	for (auto& motion : vmdMotions)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, fp);
		fread(&motion.frameNo, sizeof(motion.frameNo)
			+ sizeof(motion.location)
			+ sizeof(motion.quaternion)
			+ sizeof(motion.bezier), 1, fp);
	}

	// �{�[�����ƃL�[�t���[���̃n�b�V���e�[�u���𐶐�
	for (auto& m : vmdMotions)
	{
		_motionData[m.boneName].emplace_back(
			KeyFrame(m.frameNo, XMLoadFloat4(&m.quaternion), m.location,
				XMFLOAT2((float)m.bezier[3] / 127.0f, (float)m.bezier[7] / 127.0f),
				XMFLOAT2((float)m.bezier[11] / 127.0f, (float)m.bezier[15] / 127.0f)));
	}
}

std::unordered_map<std::string, std::vector<KeyFrame>> VMDMotionInfo::GetMotionData()
{
	return _motionData;
}