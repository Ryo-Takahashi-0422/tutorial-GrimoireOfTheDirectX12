#include <stdafx.h>

HRESULT PMDMaterialInfo::ReadPMDHeaderFile(std::string strModelPath)
{
	// PMD�w�b�_�t�@�C���̓ǂݍ���
	auto fp = fopen(strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	// �V�O�l�`�����ǂݍ���
	fread(signature, sizeof(signature), 1, fp);

	// pmd�w�b�_�[���ǂݍ���
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	// pmd���_���̓ǂݍ���
	fread(&vertNum, sizeof(vertNum), 1, fp);

	// ���_���̃T�C�Y��38�Œ�
	pmdvertex_size = 38;

	// ���_�R���e�i�̃T�C�Y�ύX�A���_���Qt_vertex��pmd�f�[�^����ǂݍ���
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	// pmd�t�@�C���̖ʒ��_���X�g���璸�_���擾�A�R���e�i�T�C�Y�ύX�A���_�ԍ��擾
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// �}�e���A���ǂݍ��݂ƃV�F�[�_�[�ւ̏o�͏���
	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMat1.resize(materialNum);
	pmdMat2.resize(materialNum);
	materials.resize(materialNum);

	for (int i = 0; i < materialNum; i++)
	{
		fread(&pmdMat1[i], 46, 1, fp);
		fread(&pmdMat2[i], sizeof(PMDMaterialSet2), 1, fp);
	}

	for (int i = 0; i < materialNum; i++)
	{
		materials[i].indiceNum = pmdMat2[i].indicesNum;
		materials[i].material.diffuse = pmdMat1[i].diffuse;
		materials[i].material.alpha = pmdMat1[i].alpha;
		materials[i].material.specular = pmdMat1[i].specular;
		materials[i].material.specularity = pmdMat1[i].specularity;
		materials[i].material.ambient = pmdMat1[i].ambient;
		materials[i].addtional.texPath = pmdMat2[i].texFilePath;
	}

	// �{�[�����ǂݍ���
	boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	pmdBones.resize(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);

	// �C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z�̂��ߌ�ŗ��p
	boneName.resize(pmdBones.size());
	_boneNameArray.resize(pmdBones.size());
	_boneNodeAddressArray.resize(pmdBones.size());

	// �{�[���m�[�h�}�b�v���쐬
	for (int idx = 0; idx < pmdBones.size(); idx++)
	{
		auto& pb = pmdBones[idx];
		boneName[idx] = pb.boneName;
		auto& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.headPos;

		//�C���f�b�N�X�����p
		_boneNameArray[idx] = pb.boneName;
		_boneNodeAddressArray[idx] = &node;
	}

	// �e�q�֌W�̍\�z
	for (auto pb : pmdBones) 
	{
		if (pb.parentIndex >= pmdBones.size())
		{
			continue;
		}

		auto pBoneName = boneName[pb.parentIndex]; // �����̐e�̖��O
		_boneNodeTable[pBoneName].children.emplace_back(&_boneNodeTable[pb.boneName]); // �e�e�[�u���̎q�Ɏ�����ǉ�
	}

	// �{�[���p�s������ׂď�����
	_boneMatrice.resize(pmdBones.size());
	std::fill(_boneMatrice.begin(), _boneMatrice.end(), XMMatrixIdentity());

	// IK�f�[�^�̐���ǂ�
	ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);
	pmdIkData.resize(ikNum);

	for (auto& ik : pmdIkData)
	{
		fread(&ik.boneidx, sizeof(ik.boneidx), 1, fp);
		fread(&ik.targetidx, sizeof(ik.targetidx), 1, fp);
		fread(&chainLen, sizeof(chainLen), 1, fp);
		ik.nodeIdx.resize(chainLen);
		fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
		fread(&ik.limit, sizeof(ik.limit), 1, fp);

		if (chainLen == 0)
		{
			continue; // �Ԃ̃m�[�h����0�Ȃ珈���I��
		}

		fread(ik.nodeIdx.data(), sizeof(ik.nodeIdx[0]), chainLen, fp);
	}

	// ���ޯ�ޗp
	auto getNameFromIdx = [&](uint16_t idx)->std::string
	{
		auto it = std::find_if(
			_boneNodeTable.begin(),
			_boneNodeTable.end(),
			[idx](const std::pair<std::string, BoneNode>& obj)->bool
			{
				return obj.second.boneIdx == idx;
			});

		if (it != _boneNodeTable.end())
		{
			return it->first;
		}

		else
		{
			return "";
		}
	};

	for (auto& ik : pmdIkData)
	{
		std::ostringstream oss;
		oss << "IK�{�[���ԍ�=" << ik.boneidx << ":" << getNameFromIdx(ik.boneidx) << std::endl;
		
		for (auto& node : ik.nodeIdx)
		{
			oss << "\t �m�[�h�{�[�� =" << node << ":" << getNameFromIdx(node) << std::endl;				
		}

		oss << "IK�^�[�Q�b�g�{�[���ԍ�=" << ik.targetidx << ":" << getNameFromIdx(ik.targetidx) << "\n" << std::endl;

		//OutputDebugStringW(oss.str().c_str());
		OutputDebugStringA(oss.str().c_str());
	}
	
	fclose(fp);
	return S_OK;
};

//std::vector<DirectX::XMMATRIX> PMDMaterialInfo::GetBoneMatrices()
//{
//	return _boneMatrice;
//}

size_t PMDMaterialInfo::GetNumberOfBones()
{
	return pmdBones.size();
}

std::map<std::string, BoneNode> PMDMaterialInfo::GetBoneNode()
{
	return _boneNodeTable;
}