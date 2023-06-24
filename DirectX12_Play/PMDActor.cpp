#include <stdafx.h>
#pragma comment(lib, "winmm.lib")

namespace
{
	enum class BoneType
	{
		Rotation,		// ��]
		RotAndMove,		// ��]���ړ�
		IK,				// IK
		Undefine,		// ����`
		IKChild,		// IK�e���{�[��
		RotationChild,	// ��]�e���{�[��
		IKDestination,	// IK�ڑ���
		Invisible		// �s���{�[��
	};
}

//PMDActor::PMDActor(PMDMaterialInfo* _pmdMatInfo, VMDMotionInfo* _vmdMotionInfo) : pmdMaterialInfo(_pmdMatInfo), vmdMotionInfo(_vmdMotionInfo)
//{
//	//boneMatrices = pmdMaterialInfo->GetBoneMatrices();
//	pmdBonesNum = pmdMaterialInfo->GetNumberOfBones();
//	boneMatrices.resize(pmdBonesNum);
//	std::fill(boneMatrices.begin(), boneMatrices.end(), XMMatrixIdentity());
//	bNodeTable = pmdMaterialInfo->GetBoneNode();
//}

PMDActor::PMDActor(PMDMaterialInfo* _pmdMatInfo, VMDMotionInfo* _vmdMotionInfo)
{
	pmdMaterialInfo = new PMDMaterialInfo;
	vmdMotionInfo = new VMDMotionInfo;

	pmdMaterialInfo = _pmdMatInfo;
	vmdMotionInfo = _vmdMotionInfo;

	//boneMatrices = pmdMaterialInfo->GetBoneMatrices();
	pmdBonesNum = pmdMaterialInfo->GetNumberOfBones();
	boneMatrices.resize(pmdBonesNum);
	std::fill(boneMatrices.begin(), boneMatrices.end(), XMMatrixIdentity());
	bNodeTable = pmdMaterialInfo->GetBoneNode();
}

void PMDActor::UpdateVMDMotion()
{
	i++;
	_duration = 0;
	for (auto& boneMotion : vmdMotionInfo->GetMotionData())
	{
		// �L�[�t���[���̏��Ԃ������ɕ��ёւ���
		std::sort(
			boneMotion.second.begin(), boneMotion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval)
			{
				return lval.frameNo <= rval.frameNo;
			}
		);

		// �ő�t���[���ԍ��擾
		_duration = std::max<unsigned int>(_duration, boneMotion.second[boneMotion.second.size() - 1].frameNo);
		auto itBoneNode = bNodeTable.find(boneMotion.first);
		if (itBoneNode == bNodeTable.end())
		{
			continue;
		}

		// ���v������̂�T��

		//auto node = bNodeTable[boneMotion.first];
		auto node = itBoneNode->second;
		auto keyFrames = boneMotion.second;
		auto rit = std::find_if(
			keyFrames.rbegin(), keyFrames.rend(),
			[this](const KeyFrame keyFrame)
			{
				return keyFrame.frameNo <= frameNo;
			});

		if (rit == keyFrames.rend())
		{
			continue;
		}

		XMMATRIX rotation;
		auto it = rit.base();
		if (it != keyFrames.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo)
				/ static_cast<float>(it->frameNo - rit->frameNo);

			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);
			// ���`���
			//rotation = XMMatrixRotationQuaternion(rit->quaternion) * (1 - t)
			//	+ XMMatrixRotationQuaternion(it->quaternion) * t;

			// ���ʐ��`���
			rotation = XMMatrixRotationQuaternion(XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		boneMatrices[node.boneIdx] = mat;
	}
}

void PMDActor::RecursiveMatrixMultiply(const DirectX::XMMATRIX& mat)
{
	auto node = &bNodeTable["�Z���^�["];
	boneMatrices[node->boneIdx] *= mat;

	for (auto cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, boneMatrices[node->boneIdx]);
	}
}

void PMDActor::RecursiveMatrixMultiply(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	boneMatrices[node->boneIdx] *= mat;

	for (auto cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, boneMatrices[node->boneIdx]);
	}
}

XMMATRIX PMDActor::LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
{
	// ��������������(Z���x�N�g��)
	XMVECTOR vz = lookat;

	// (�������������������������Ƃ���)��Y��
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

	// �������������������������Ƃ���X�����v�Z
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));

	// �^��Y�����v�Z
	vy = XMVector3Cross(vz, vx);

	// ����LookAt��up�����������������Ă�����right����ɂ��č�蒼��
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		// (�������������������������Ƃ���)��X�x�N�g��
		vx = XMVector3Normalize(XMLoadFloat3(&right));

		// �������������������������Ƃ���Y�����v�Z
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		//�^��X�����v�Z
		vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	}

	XMMATRIX ret = XMMatrixIdentity();
	ret.r[0] = vx;
	ret.r[1] = vy;
	ret.r[2] = vx;
	return ret;
}

XMMATRIX PMDActor::LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right) {
	return XMMatrixTranspose(LookAtMatrix(origin, up, right)) *	LookAtMatrix(lookat, up, right);
}

void PMDActor::IKSolve()
{
	for (auto& ik : _ikData)
	{
		auto childrenNodeCount = ik.nodeIdx.size();
		switch (childrenNodeCount)
		{
		case 0: // �Ԃ̃{�[����0(�P�[�X����)
			continue;
		case 1: // �Ԃ̃{�[����1�̎���LookAt
			SolveLookAtIK(ik);
		case 2: // �Ԃ̃{�[����2�̎��͗]���藝
			SolveCosineIK(ik);
		default: // �Ԃ̃{�[����3�̎���CCD-IK
			SolveCCDIK(ik);
		}
	}
}

void PMDActor::SolveCCDIK(const PMDIK& ik)
{

}

void PMDActor::SolveCosineIK(const PMDIK& ik)
{

}

void PMDActor::SolveLookAtIK(const PMDIK& ik) 
{
	// root��target�֌������x�N�g���𓱏o
	auto rootNode = pmdMaterialInfo->_boneNodeAddressArray[ik.nodeIdx[0]];
	auto targetNode = pmdMaterialInfo->_boneNodeAddressArray[ik.targetidx];

	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto rpos2 = XMVector3TransformCoord(rpos1, boneMatrices[ik.nodeIdx[0]]);
}

void PMDActor::PlayAnimation() 
{
	_startTime = timeGetTime();
}

void PMDActor::MotionUpdate(unsigned int maxFrameNum)
{
	elapsedTime = timeGetTime() - _startTime; // �o�ߎ��Ԃ𑪒肵�Ċi�[
	frameNo = 30 * (elapsedTime / 1000.0f);

	if (frameNo > maxFrameNum)
	{
		PlayAnimation();
		frameNo = 0;
	}
}

unsigned int PMDActor::GetFrameNo()
{
	return frameNo;
}

float PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x; // �v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3�W��
	const float k1 = 3 * b.x - 6 * a.x; // t^2�W��
	const float k2 = 3 * a.x; // t�W��

	// �덷�͈͓̔����ǂ������肷�邽�߂̒萔
	constexpr float epsilon = 0.0005f;

	// t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i) 
	{
		// f(t)�����߂�
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// �������ʂ�0�ɋ߂�(�덷�͈͓̔�)�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // ����������
	}

	// ���߂���t�͊��ɋ��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.x + 3 * t * r * r * a.x;
}

std::vector<DirectX::XMMATRIX>* PMDActor::GetMatrices()
{
	return &boneMatrices;
}