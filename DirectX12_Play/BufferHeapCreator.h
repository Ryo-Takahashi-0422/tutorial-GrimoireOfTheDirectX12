#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;
	ComPtr<ID3D12Resource> vertBuff = nullptr;
	ComPtr<ID3D12Resource> idxBuff = nullptr;//CPUとGPUの共有?バッファー領域(リソースとヒープ)
	ComPtr<ID3D12Resource> depthBuff = nullptr; // デプスバッファー
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // 行列用定数バッファー
	ComPtr<ID3D12Resource> materialBuff = nullptr; // マテリアル用定数バッファー

	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//テクスチャCPUアップロード用バッファー
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//テクスチャGPU読み取り用バッファー
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph用バッファー
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa用バッファー
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//トゥーン用アップロードバッファー
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//トゥーン用リードバッファー
	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string toonFilePath;
	HRESULT result;

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	void SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps);
	void SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps);
	void SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc);
	void SetClearValue(D3D12_CLEAR_VALUE* depthClearValue);

	// 頂点バッファーの作成
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& vertresDesc);

	// インデックスバッファーの作成
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& indicesDesc);

	// デプスバッファーの作成
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& depthResDesc);

	// 行列用定数バッファーの作成
	HRESULT CreateConstBufferOfMatrix(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& constBuffProp, D3D12_RESOURCE_DESC& constBuffResdesc);

	// マテリアル用定数バッファーの作成
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& materialHeapProp, D3D12_RESOURCE_DESC& materialBuffResDesc);

	// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	void CreateUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& metaData,
		std::vector<DirectX::Image*>& img);

	// Toonテクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	void CreateToonUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& toonMetaData,
		std::vector<DirectX::Image*>& toonImg);

	ComPtr<ID3D12Resource> GetVertBuff();
	ComPtr<ID3D12Resource> GetIdxBuff();
	ComPtr<ID3D12Resource> GetDepthBuff();
	ComPtr<ID3D12Resource> GetMatrixBuff();
	ComPtr<ID3D12Resource> GetMaterialBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexReadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetsphMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetspaMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonReadBuff();
};