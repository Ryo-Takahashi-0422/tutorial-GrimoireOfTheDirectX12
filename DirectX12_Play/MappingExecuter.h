#pragma once

class MappingExecuter
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	// マッピング先ポインタ群
	unsigned char* vertMap = nullptr;
	unsigned short* mappedIdx = nullptr;
	char* mapMaterial = nullptr;
	uint8_t* mapforImg = nullptr;
	uint8_t* toonmapforImg = nullptr;

	HRESULT result;
public:
	MappingExecuter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);

	//頂点バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的に頂点データをコピーする。
	void MappingVertBuff();

	//インデクスバッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	void MappingIndexOfVertexBuff();

	//マテリアル用バッファーへのマッピング
	void MappingMaterialBuff();

	// 以下二つはマッピング先が違うのみで他は同じ処理。どうにかして統一したいが...
	// テクスチャアップロード用バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	void TransferTexUploadToBuff(std::vector<DirectX::Image*> img);
	// トゥーンテクスチャアップロード用バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	void TransferToonTexUploadToBuff(std::vector<DirectX::Image*> toonImg);


};