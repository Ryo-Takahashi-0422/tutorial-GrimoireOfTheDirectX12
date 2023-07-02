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
	kneeIdxs = pmdMaterialInfo->GetKneeIdx();
	ikData = pmdMaterialInfo->GetpPMDIKData();
	boneNodeAddressArray = pmdMaterialInfo->GetBoneNodeAddressArray();
	for (auto& ik : ikData) 
	{
		if (ik.nodeIdx.size() == 1)
		{
			lookAtSolvedIK.push_back(ik.nodeIdx[0]);
		}		
	}	

	epsilon = 0.005f;
}

void PMDActor::UpdateVMDMotion()
{
	_duration = 0;
	//�s����N���A(���ĂȂ��ƑO�t���[���̃|�[�Y���d�ˊ|������ă��f��������)
	std::fill(boneMatrices.begin(), boneMatrices.end(), XMMatrixIdentity());
	
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

		XMMATRIX rotation = XMMatrixIdentity();
		XMVECTOR offset = XMLoadFloat3(&rit->location);

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
			offset = XMVectorLerp(offset, XMLoadFloat3(&it->location), t);
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		auto offsetMat = XMMatrixTranslationFromVector(offset);
		boneMatrices[node.boneIdx] = mat * offsetMat;
		//boneMatrices[node.boneIdx] = mat * XMMatrixTranslationFromVector(offset);

		// debug
		if (node.boneIdx == 83 & frameNo > 65)
		{
			int i = 0;
		}
	}
	RecursiveMatrixMultiply(&bNodeTable["�Z���^�["], XMMatrixIdentity());
	IKSolve();
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
	ret.r[2] = vz;
	return ret;
}

XMMATRIX PMDActor::LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right) {
	// �t�s��͑O���LookAt�s��ɂ���]��ł��������߁B�����ĉ��߂ē��o�����s���LookAt���Ă���B
	return XMMatrixTranspose(LookAtMatrix(origin, up, right)) *	LookAtMatrix(lookat, up, right);
}

void PMDActor::IKSolve()
{
	// Y���ړ��̂݁AZ���ړ��݂̂ɑΉ��B(����ȏ�͉�]���̉��P(�N�H�[�^�j�I���Ή�?)���K�v��...)
	for (auto& ik : ikData)
	{
		auto childrenNodeCount = ik.nodeIdx.size();
		switch (childrenNodeCount)
		{
		case 0: // �Ԃ̃{�[����0(�P�[�X����)
			continue;
		case 1: // �Ԃ̃{�[����1�̎���LookAt
			SolveLookAtIK(ik);
			break;
		case 2: // �Ԃ̃{�[����2�̎��͗]���藝
			SolveCosineIK(ik);
			break;
		default: // �Ԃ̃{�[����3�̎���CCD-IK
			SolveCCDIK(ik);
		}
	}
}

void PMDActor::SolveCCDIK(const PMDIK& ik)
{
	// IK�{�[��(�������{�[��)�̊e�ϐ�
	auto targetBoneNode = boneNodeAddressArray[ik.boneidx];
	auto targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	auto parentMat = boneMatrices[boneNodeAddressArray[ik.boneidx]->ikParentBone];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, parentMat);

	// invParentsMat�ɂ��ψʂ���IK�{�[���̕ψʂ��F���ɂȂ�B�����"�����~�N.pmd"�ɂ����郊�O�\���ł͔���ȸ��IK�{�[����
	// �΂��ăZ���^�[�{�[�����e�ƂȂ��Ă���A���̃Z���^�[�{�[���̕ψʂɂ��e�����t�s���Z�ɂ��ł��������ƂɂȂ�A���̌��ʂ�
	// bornMatrices[]�ɏ㏑�����Ă��邩��B���̊֐��ɓ���O�ɁARecursive...�ɂ���Z���ꂽ�s�񂪊i�[����Ă���)�B
	// boneMatrices[0] * invParentMat�ɂ��ψʂ�0�ł���B=targetOriginPos�ł��邪�AbornMatrices[]�ƌ��т��Ă��Ȃ�����
	// Recursive...()�ɂ�鉉�Z�ɂ��Z���^�[�{�[���̎q�ł���IK�{�[���͕ψʂ���B����ɂ�����IK children�B���Z���^�[�{�[��
	// �ɂ��ψʂƕ�������Ă���
	//auto targetNextPos = XMVector3Transform(targetOriginPos, invParentMat);
	//auto targetNextPos = XMVector3Transform(targetOriginPos, boneMatrices[0] * invParentMat);
	auto targetNextPos = targetOriginPos;

	// ���[�{�[���̏����ʒu
	auto endPos = XMLoadFloat3(&boneNodeAddressArray[ik.targetidx]->startPos);

	bonePositions.clear();
	// ���ԃ{�[���̏����ʒu
	for (auto& cidx : ik.nodeIdx)
	{
		bonePositions.push_back(XMLoadFloat3(&boneNodeAddressArray[cidx]->startPos));
	}

	// ��]�s��̋L�^�p�B4*4�̒P�ʍs��Ŗ��߂Ă����B
	std::vector<XMMATRIX> mats(bonePositions.size());
	fill(mats.begin(), mats.end(), XMMatrixIdentity());

	auto ikLimit = ik.limit * XM_PI; // PMD�f�[�^�ł͊p�x=�x���@/180���ŕ\������Ă���̂Œ������Ă���B

	// ik�ɐݒ肳��Ă��鎎�s�񐔌J��Ԃ�CCD�v�Z���s��
	for (int c = 0; c < ik.iterations; ++c)
	{
		// ���[�{�[���ƃ^�[�Q�b�g(IK�{�[��)����v������C�e���[�V�����𔲂���
		if (XMVector3Length(
			XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		// ���ꂼ��̃{�[����k��Ȃ���p�x�����ɒ�G���Ȃ��悤�ɋȂ��Ă���
		// bonePositions��CCD-IK�ɂ�����e�m�[�h�̍��W���x�N�^�z��ɂ�������
		for (int bidx = 0; bidx < bonePositions.size(); ++bidx)
		{
			const auto& pos = bonePositions[bidx];
			// �Ώۃ{�[�����疖�[�{�[���܂łƑΏۃx�N�g������
			// �^�[�Q�b�g�܂ł̃x�N�g���쐬
			auto vecToEnd = XMVectorSubtract(endPos, pos); // �Ώۃ{�[�������[�{�[��
			auto vecToTarget = XMVectorSubtract(targetNextPos, pos); // �Ώۃ{�[�����^�[�Q�b�g(IK)

			// ���K��
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			// �قړ����x�N�g���ɂȂ����ꍇ�͊O�ς��v�Z�ł��Ȃ��̂Ŏ��̏�����
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// �O�ϋy�ъp�x�v�Z
			auto cross = XMVector3Cross(vecToEnd, vecToTarget);
			cross = XMVector3Normalize(cross);

			float angle = XMVector2AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0]; // 0���`90���A-90���`0���̔��ʕs�\

			// ��]���E�𒴂����Ƃ��̕␳
			angle = min(angle, ikLimit);

			// �O�ςƊp�x�����]�s��̎Z�o
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle);

			// ���_�ł͂Ȃ�pos���S�ɉ�]�����邽�߁A��]�s��̑O��ɕ��s�ړ��s��̋t�s��A�s�����Z����B
			// �Ⴆ��ȸ��3�̏�������-pos����ƁAȸ��3�̍��W�����_�Ɉړ����邱�ƂɂȂ�A����ɂ����pos���S�̉�]�Ƃ����Ӗ��ɂȂ�B
			auto mat = 
				XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// �s��̋L�^
			mats[bidx] *= mat;

			// �s��̓K�p�͌��݂̃{�[������݂Ė��[���̒��ԃ{�[�����Ώ�
			for (int idx = bidx - 1; idx >= 0; --idx)
			{
				bonePositions[idx] = XMVector3Transform(bonePositions[idx], mat);
			}

			// ���[�{�[���ɂ��s���K�p
			endPos = XMVector3Transform(endPos, mat);

			// ���[��IK�̃x�N�g������臒l�����Ȃ珈���I��
			if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
			{
				break;
			}				
		}
	}

	// �v�Z�ߒ��Ŋi�[���Ă������s���boneMatrices�̊e�A�h���X�ɏ㏑�����Ă���
	// �Z���^�[�{�[���ɂ��e�����l�����Ă��Ȃ����ʂɂȂ��Ă���B
	int idx = 0;
	for (auto& cidx : ik.nodeIdx)
	{
		boneMatrices[cidx] = mats[idx];
		++idx;
	}
	
	// ���[�g�{�[�����疖�[�{�[����IK�{�[���̐e�{�[��("�����~�N.pmd"�ł́u�Z���^�[�v�{�[��)�s�����Z���āA
	// ���̌��ʂ��ċA�I�ɏ�Z���Ă����B�܂�A�Z���^�[�{�[�����l�����Ȃ����ʂ��Z���^�[�{�[���ɂ��e���ƌ��т��邱�ƂŁA
	// ����ׂ�����(�s��)���{�[���ɓK�p���Ă����B
	auto rootNode = pmdMaterialInfo->GetBoneNodeAddressArray()[ik.nodeIdx.back()];
	RecursiveMatrixMultiply(rootNode, parentMat);
}

void PMDActor::SolveCosineIK(const PMDIK& ik)
{
	// �^�[�Q�b�g��IK�{�[��(���[�{�[���ł͂Ȃ��A���[�{�[�����߂Â��ڕW�{�[��(IK�{�[��)�̍��W���擾)
	auto& targetNode = boneNodeAddressArray[ik.boneidx];
	// IK�{�[���̕ψʌ���W���i�[
	auto targetPos = XMVector3Transform(XMLoadFloat3(&targetNode->startPos), (boneMatrices[ik.boneidx]));

	// ���[�{�[��
	auto endNode = boneNodeAddressArray[ik.targetidx];
	positions.clear();
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));
	//positions.emplace_back(XMVector3Transform(XMLoadFloat3(&endNode->startPos), boneMatrices[ik.boneidx]));

	// ���ȏ��ɖ��������̂Œǉ��B���[�{�[���s��ɂ͊֐��ɓ���O�ɃZ���^�[�{�[���̉�]�ړ��s�����Z����Ă���̂ŁA���E���Ă����B
	auto parentMat = boneMatrices[0];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, parentMat);
	boneMatrices[ik.targetidx] *= invParentMat;

	auto targetNextPos = XMVector3Transform(XMLoadFloat3(&boneNodeAddressArray[ik.boneidx]->startPos), boneMatrices[ik.boneidx]);
	//targetPos = targetNextPos;

	// ���ԋy�у��[�g�m�[�h(MMD�f�[�^�̍\����A�Q�Ƃ͒��ԁ����[�g�̏��ɂȂ�)
	for (auto& chainBoneNode : ik.nodeIdx)
	{
		auto boneNode = boneNodeAddressArray[chainBoneNode];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// positions�����̃f�[�^�������[�����ԁ����[�g�m�[�h�Ȃ̂ŁA������₷�����邽�ߋt���ɂ���
	std::reverse(positions.begin(), positions.end()); // ���[�g�����ԁ����[�̏��ɕύX
	// ��Ŏg���̂ŏ����l�ۑ����Ă���
	auto p0s = positions[0];
	auto p2s = positions[2];

	// ���̒����𑪒�
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0]; // ���[�g�����ԃx�N�g���̒���
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0]; // ���ԁ����[�x�N�g���̒���

	// ���[�g�{�[�����W�ϊ�
	positions[0] = XMVector3Transform(positions[0], boneMatrices[ik.nodeIdx[1]]); // ik.nodeIdx[1]�̓��[�g�{�[�����w���Ă���
	// positions[1]�͎����I�ɓ��o�����
	// 
	// ���[�{�[��
	positions[2] = XMVector3Transform(positions[2], boneMatrices[ik.boneidx]); // ���[�{�[����ik�{�[���s��ō��W�ϊ����āA�ړI�ʒu��IK�Ɠ��l�ɂ���
	//positions[2] = XMVector3Transform(positions[2], boneMatrices[ik.targetidx]); // ik.nodeIdx[2]�͑��݂��Ȃ��̂ł́H��targetIdx��

	// ���[�g���疖�[�ւ̃x�N�g��(���W�ϊ���B�����ʒu�ł͂Ȃ����Ƃɒ���)������Ă���
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);
	
	//float A = XMVector3Length(linearVec).m128_f32[0]; // ���W�ϊ���̃��[�g�����[�x�N�g������
	float B = edgeLens[0]; // ���[�g�����ԃx�N�g���̒���
	float C = edgeLens[1]; // ���ԁ����[�x�N�g���̒���
	float A = min(B + C - 0.01f, XMVector3Length(linearVec).m128_f32[0]); // ���W�ϊ���̃��[�g�����[�x�N�g������

	// ���W�ϊ���Ƀ��[�g�����ԃx�N�g���ƃ��[�g�����[�x�N�g���̐����p�x��1
	float x1 = (A * A + B * B - C * C) / (2 * A * B);
	float theta1 = acosf(x1);

	// ���W�ϊ���Ƀ��[�g�����ԃx�N�g���ƒ��ԁ����[�x�N�g���̐����p�x��2
	float x2 = (B * B + C * C - A * A) / (2 * B * C);
	float theta2 = acosf(x2);

	// IK�{�[����Z�����̕ψʂ�����ꍇ�̏���
    // ���ȏ��x�[�X�ł�ik��z�����ړ��L�胂�[�V�������Č��o���Ȃ��̂œƎ������Ƃ��Ēǉ�
	XMMATRIX rotationOfIKzMove = XMMatrixIdentity();
	XMVECTOR root2IK;
	XMVECTOR root2TargetStart;
	XMVECTOR rootMinusroot2IK;
	float theta3 = 0;
	if (frameNo > 1)
	{
		// ���ԃm�[�h��ik�{�[�����߂Â��Ă�����A���[�g�{�[�������_�Ƃ��ă��[�g��IK�̊p�x�����[�g�E���ԁE���[�{�[������]������
		float f = beforeIKMat.r[3].m128_f32[2] - boneMatrices[ik.boneidx].r[3].m128_f32[2];
		if (f > 0) 
		{
			root2IK = XMVectorSubtract(targetPos, p0s);//positions[0]);
			root2TargetStart = XMVectorSubtract(XMLoadFloat3(&targetNode->startPos), p0s);//positions[0]);
			rootMinusroot2IK = XMVectorSubtract(root2IK, root2TargetStart);
			float D = XMVector3Length(root2IK).m128_f32[0];
			float E = XMVector3Length(rootMinusroot2IK).m128_f32[0];
			float y = (B * B + D * D - E * E) / (2 * B * D);
			y = min(1, y); // 1�������arccos�l���v�Z�ł��Ȃ�
			theta3 = acosf(y);
		}

		// ���ԃm�[�h����ik�{�[����������������A���[�g�{�[�������_�Ƃ��ă��[�g��IK�̊p�x�����[�g�E���ԁE���[�{�[�����t��]������
		else if(f < 0)
		{
			root2IK = XMVectorSubtract(targetPos, p0s);//positions[0]);
			root2TargetStart = XMVectorSubtract(XMLoadFloat3(&targetNode->startPos), p0s);//positions[0]);
			rootMinusroot2IK = XMVectorSubtract(root2IK, root2TargetStart);
			float D = XMVector3Length(root2IK).m128_f32[0];
			float E = XMVector3Length(rootMinusroot2IK).m128_f32[0];
			float y = (B * B + D * D - E * E) / (2 * B * D);
			y = min(1, y); // 1�������arccos�l���v�Z�ł��Ȃ�
			theta3 = -acosf(y);
		}
	}

	// �������߂�
	// �����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���
	XMVECTOR axis;
	if (find(kneeIdxs.begin(),
		kneeIdxs.end(),
		ik.nodeIdx[0]) == kneeIdxs.end())
	{
		auto vm = XMVector3Normalize(XMVectorSubtract(positions[2], positions[0])); // ���[�g�����[�x�N�g��
		auto vt = XMVector3Normalize(XMVectorSubtract(targetPos, positions[0])); // ���[�g��IK�{�[���x�N�g��
		axis = XMVector3Cross(vt, vm);
	}

	else
	{
		auto right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	// Z������IK���ړ������ꍇ�̍s��
	rotationOfIKzMove = XMMatrixTranslationFromVector(-positions[0]);
	rotationOfIKzMove *= XMMatrixRotationAxis(axis, theta3);
	rotationOfIKzMove *= XMMatrixTranslationFromVector(positions[0]);

	// Debug
	//if (frameNo > 79)
	//{
	//	int i = 0;
	//}

	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2 - XM_PI*1.001);  // ���ԃ{�[��=�Ђ��ƍl����ƕ�����₷���B�Ȃ����p�x�͋Ȃ�������̊p�x-180��
	mat2 *= XMMatrixTranslationFromVector(positions[1]);

	boneMatrices[ik.nodeIdx[1]] *= rotationOfIKzMove * mat1; // ���[�g�{�[���s��̓��[�g�{�[���̉�]�s��
	boneMatrices[ik.nodeIdx[0]] = mat2 * boneMatrices[ik.nodeIdx[1]]; // ���ԃ{�[���s��͒��ԃ{�[���y�у��[�g�{�[���̉�]�s�����Z�������̂�
	
	// "�����~�N.pmd"��p�����B��IK�̈ړ��ɂ��������Ăܐ�IK���ړ�������B
	// IK�{�[���̎q�{�[���ɍs��K�p
	for (auto& childIK : boneNodeAddressArray[ik.boneidx]->children)
	{
		boneMatrices[childIK->boneIdx] *= boneMatrices[ik.boneidx];

	}

	 // ���ۂ͖��[�{�[���s��̓��[�g�E���ԃ{�[���s��̉e�����󂯂�B�����Ă���PMD���f���̃��O�̑g�ݕ��ɂ���āA����{�[����
	 // ���ꂼ��2��IK�`�F�[���ɑg�ݍ��܂�Ă���B���ꂼ��LookAtIK��CosineIK�Ώۂł���BLookAt���ł̌��ʂ�Cosine�ł̈ȉ��v�Z���ʂ�
	 // �㏑������邱�ƂŁA����{�[�����]�܂Ȃ����������Ă��܂����߁A�������Ȃ����ƂŐ����������킹�Ă���B�Ǝ��́A���̏ꂵ�̂���
	 // �����ŉ��P���K�v�Ǝv����B
	auto isExsist = find(lookAtSolvedIK.begin(), lookAtSolvedIK.end(), ik.targetidx) != lookAtSolvedIK.end();
	if (!isExsist)
	{
		boneMatrices[ik.targetidx] = boneMatrices[ik.nodeIdx[0]];
	}
	else
	{
		//boneMatrices[ik.targetidx] = boneMatrices[ik.nodeIdx[0]];// *XMMatrixTranspose(XMMatrixRotationAxis(axis, theta1) * XMMatrixRotationAxis(axis, theta2 - XM_PI));
		boneMatrices[ik.targetidx] *= boneMatrices[ik.boneidx];//boneMatrices[ik.nodeIdx[0]] * XMMatrixTranslationFromVector(targetNextPos);
	}
	
	// ���̏����Ɍ����ď��̕ۑ�
	befTheta1 = theta1;
	beforeIKMat = boneMatrices[ik.boneidx];

	// ���[�{�[���̎q�{�[���ɍs��K�p
	for (auto& childIK : boneNodeAddressArray[ik.targetidx]->children)
	{
		boneMatrices[childIK->boneIdx] *= boneMatrices[ik.boneidx];		
	}	
}

void PMDActor::SolveLookAtIK(const PMDIK& ik) 
{
	// root�F���[�g�{�[���Atarget�F���[�{�[��
	auto rootNode = boneNodeAddressArray[ik.nodeIdx[0]];
	auto targetNode = boneNodeAddressArray[ik.targetidx];

	// ��������x�N�g��(���[�g�����[�ւ̕����x�N�g���𓱏o���ĒP�ʃx�N�g����)
	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);
	auto originVec = XMVectorSubtract(tpos1, rpos1);
	originVec = XMVector3Normalize(originVec);

	// �C�ӕ����x�N�g��(���[�g/���[�̍��W�Ɋe�X�̃m�[�h���ێ����錻�݂̉�]�E���s�ړ��s�����Z�����ψʌ�̍��W����A
	// �����x�N�g���𓱏o���ĒP�ʃx�N�g����)
	auto rpos2 = XMVector3TransformCoord(rpos1, boneMatrices[ik.nodeIdx[0]]);
	auto tpos2 = XMVector3TransformCoord(tpos1, boneMatrices[ik.boneidx]); // ���[��IK�Ɉړ���������(���[�g��IK�x�N�g������肽��)����IK�s�����Z����	
	auto targetVec = XMVectorSubtract(tpos2, rpos2);
	targetVec = XMVector3Normalize(targetVec);

	XMFLOAT3 up = XMFLOAT3(0, 1, 0); // ��x�N�g��
	XMFLOAT3 right = XMFLOAT3(1, 0, 0); // �E�x�N�g��
	auto mat = 
		XMMatrixTranslationFromVector(-rpos2)
		* LookAtMatrix(originVec, targetVec, up, right)
		* XMMatrixTranslationFromVector(rpos2);

	boneMatrices[ik.nodeIdx[0]] *= mat;

	// IK�{�[���̎q�{�[���ɍs��K�p
	for (auto& childIK : boneNodeAddressArray[ik.boneidx]->children)
	{
		boneMatrices[childIK->boneIdx] *= boneMatrices[ik.boneidx];

	}
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

unsigned int PMDActor::GetDuration()
{
	return _duration;
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