#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> matrixHeap = nullptr; // 行列CBV, SRVディスクリプタヒープ

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

	ComPtr<ID3D12Resource> _peraResource; // マルチパスレンダリング用書き込み先リソース
	ComPtr<ID3D12DescriptorHeap> _peraRTVHeap; // レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> _peraSRVHeap; // テクスチャ用

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	void SetRTVHeapDesc(D3D12_DESCRIPTOR_HEAP_DESC* rtvHeapDesc); // RTV用ヒープ詳細設定
	void SetDSVHeapDesc();//D3D12_DESCRIPTOR_HEAP_DESC* dsvHeapDesc); // DSV用ヒープ詳細設定
	void SetMatrixHeapDesc(D3D12_DESCRIPTOR_HEAP_DESC* matrixHeapDesc); // 行列用ヒープ詳細設定
	void SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps); // 頂点・インデックス用ヒーププロパティ設定
	void SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps); // 深度用ヒーププロパティ設定
	void SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc); // 深度用リソース詳細設定
	void SetClearValue(D3D12_CLEAR_VALUE* depthClearValue); // クリアバリュー設定

	// RTVヒープの作成
	HRESULT CreateRTVHeap(ComPtr<ID3D12Device> _dev, D3D12_DESCRIPTOR_HEAP_DESC& rtvHeapDesc);

	// DSVヒープの作成
	HRESULT CreateDSVHeap(ComPtr<ID3D12Device> _dev);//, D3D12_DESCRIPTOR_HEAP_DESC& dsvHeapDesc);

	// 行列用ヒープの作成
	HRESULT CreateMatrixHeap(ComPtr<ID3D12Device> _dev, D3D12_DESCRIPTOR_HEAP_DESC& matrixHeapDesc);

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

	// マルチパスレンダリング用のバッファ作成
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& mutipassHeapProp,
		D3D12_RESOURCE_DESC& mutipassResDesc, D3D12_CLEAR_VALUE clearValue);

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
	
	ComPtr<ID3D12DescriptorHeap> GetRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMatrixHeap();
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