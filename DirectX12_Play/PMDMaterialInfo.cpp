#include <stdafx.h>

HRESULT PMDMaterialInfo::ReadPMDHeaderFile()
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

	fclose(fp);
	return S_OK;
};