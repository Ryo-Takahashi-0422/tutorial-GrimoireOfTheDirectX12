#pragma once
using namespace DirectX;

#pragma pack(1)
struct VMDMotion
{
	char boneName[15]; // �{�[����
	unsigned int frameNo; // �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
	XMFLOAT3 location; // �ʒu
	XMFLOAT4 quaternion; // Quaternion // ��]
	unsigned char bezier[64]; // [4][4][4] // �⊮
};
#pragma pack()

// ���[�V�����\����
struct KeyFrame
{
	unsigned int frameNo;
	XMVECTOR quaternion;
	DirectX::XMFLOAT2 p1, p2; // �x�W�F�Ȑ��̒��Ԑ���_

	KeyFrame(unsigned int fno, XMVECTOR q, XMFLOAT2 ip1, XMFLOAT2 ip2) : frameNo(fno), quaternion(q), p1(ip1), p2(ip2)
	{}
};

class VMDMotionInfo
{
private:
	unsigned int motionDataNum;
	std::vector<VMDMotion> vmdMotions;
	std::unordered_map<std::string, std::vector<KeyFrame>> _motionData;

public:
	HRESULT ReadVMDHeaderFile(std::string);
	std::unordered_map<std::string, std::vector<KeyFrame>> GetMotionData();
};