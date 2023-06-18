#include <stdafx.h>

HRESULT PMDMaterialInfo::ReadPMDHeaderFile(std::string strModelPath)
{
	// PMDヘッダファイルの読み込み
	auto fp = fopen(strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//エラー処理
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	// シグネチャ情報読み込み
	fread(signature, sizeof(signature), 1, fp);

	// pmdヘッダー情報読み込み
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	// pmd頂点情報の読み込み
	fread(&vertNum, sizeof(vertNum), 1, fp);

	// 頂点情報のサイズは38固定
	pmdvertex_size = 38;

	// 頂点コンテナのサイズ変更、頂点情報群t_vertexをpmdデータから読み込み
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	// pmdファイルの面頂点リストから頂点数取得、コンテナサイズ変更、頂点番号取得
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// マテリアル読み込みとシェーダーへの出力準備
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

	// ボーン数読み込み
	boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	pmdBones.resize(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);

	// インデックスと名前の対応関係構築のため後で利用
	boneName.resize(pmdBones.size());

	// ボーンノードマップを作成
	for (int idx = 0; idx < pmdBones.size(); idx++) 
	{
		auto& pb = pmdBones[idx];
		boneName[idx] = pb.boneName;
		auto& node = _boneNodeTable[pb.boneName];
		node.boneIdx = idx;
		node.startPos = pb.headPos;
	}

	// 親子関係の構築
	for (auto pb : pmdBones) 
	{
		if (pb.parentIndex >= pmdBones.size())
		{
			continue;
		}

		auto pBoneName = boneName[pb.parentIndex]; // 自分の親の名前
		_boneNodeTable[pBoneName].children.emplace_back(&_boneNodeTable[pb.boneName]); // 親テーブルの子に自分を追加
	}

	// ボーン用行列をすべて初期化
	_boneMatrice.resize(pmdBones.size());
	std::fill(_boneMatrice.begin(), _boneMatrice.end(), XMMatrixIdentity());

	fclose(fp);
	return S_OK;
};

std::vector<DirectX::XMMATRIX> PMDMaterialInfo::GetBoneMatrices()
{
	return _boneMatrice;
}

std::map<std::string, BoneNode> PMDMaterialInfo::GetBoneNode()
{
	return _boneNodeTable;
}