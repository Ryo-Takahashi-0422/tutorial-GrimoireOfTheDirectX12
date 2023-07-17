#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; // レンダーターゲットビュー用ディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // 深度ステンシルビュー用ディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC cbvsrvHeapDesc = {}; // 行列用ディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC mutipassRTVHeapDesc = {}; // マルチパスRTV用ディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC mutipassSRVHeapDesc = {}; // マルチパスSRV用ディスクリプタヒープ

	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr; // リソースターゲットビュー用ディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr; // 深度ステンシルビュー用ディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> cbvsrvHeap = nullptr; // 行列CBV, SRV用ディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> multipassRTVHeap = nullptr; // マルチパスレンダーターゲット用ディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> multipassSRVHeap = nullptr; // マルチパステクスチャ用ディスクリプタヒープ

	D3D12_HEAP_PROPERTIES vertexHeapProps = {}; // 頂点、頂点インデックス用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProps = {}; // 深度用ヒーププロパティ
	D3D12_HEAP_PROPERTIES wvpHeapProp = {}; // 行列用ヒーププロパティ
	D3D12_HEAP_PROPERTIES materialHeapProp = {};  // PMDMaterialInfo->MaterialForHlsl定義に合わせている
	D3D12_HEAP_PROPERTIES mutipassHeapProp = {}; // マルチパス用ヒーププロパティ

	D3D12_RESOURCE_DESC vertresDesc; // 頂点用リソース詳細
	D3D12_RESOURCE_DESC indicesDesc; // 頂点インデックス用リソース詳細
	D3D12_RESOURCE_DESC depthResDesc = {}; // 深度リソース詳細
	D3D12_RESOURCE_DESC wvpResdesc = {}; // 行列用リソース詳細
	D3D12_RESOURCE_DESC materialBuffResDesc = {}; // PMDMaterialInfo->MaterialForHlsl定義に合わせている
	// ﾏﾙﾁﾊﾟｽﾊﾞｯﾌｧ用のオブジェクトはAppD3DX12が生成しているため保持しない

	ComPtr<ID3D12Resource> vertBuff = nullptr; // 頂点用バッファ
	ComPtr<ID3D12Resource> idxBuff = nullptr; // 頂点インデックス用バッファ
	ComPtr<ID3D12Resource> depthBuff = nullptr; // デプスバッファー
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // 行列用定数バッファー
	ComPtr<ID3D12Resource> materialBuff = nullptr; // マテリアル用定数バッファー
	ComPtr<ID3D12Resource> multipassBuff = nullptr; // マルチパスレンダリング用書き込み先バッファー
	ComPtr<ID3D12Resource> multipassBuff2 = nullptr; // マルチパスレンダリング用書き込み先バッファーその2
	ComPtr<ID3D12Resource> whiteTextureBuff = nullptr; // 白テクスチャ用バッファー
	ComPtr<ID3D12Resource> blackTextureBuff = nullptr; // 黒テクスチャ用バッファー
	ComPtr<ID3D12Resource> grayTextureBuff = nullptr; // グレーテクスチャ用バッファー

	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//テクスチャCPUアップロード用バッファー
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//テクスチャGPU読み取り用バッファー
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph用バッファー
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa用バッファー
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//トゥーン用アップロードバッファー
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//トゥーン用リードバッファー

	D3D12_CLEAR_VALUE depthClearValue = {}; // 深度クリアバリュー

	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string toonFilePath; // トゥーンテクスチャ保存フォルダのパス
	HRESULT result;

	unsigned long materialBuffSize;

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	// RTV用ディスクリプタヒープ設定
	void SetRTVHeapDesc();
	// DSV用ディスクリプタヒープ設定
	void SetDSVHeapDesc();
	// CBV,SRV用ディスクリプタヒープ設定(行列CBV、ﾏﾃﾘｱﾙCBV、ﾃｸｽﾁｬSRVをセット)
	void SetCBVSRVHeapDesc();
	// マルチパスRTV用ディスクリプタヒープ設定
	void SetMutipassRTVHeapDesc();
	// マルチパスSRV用ディスクリプタヒープ設定
	void SetMutipassSRVHeapDesc();
	// 頂点・インデックス用ヒーププロパティ設定
	void SetVertexHeapProp();
	// 深度用ヒーププロパティ設定
	void SetDepthHeapProp();
	// 深度用リソース詳細設定
	void SetDepthResourceDesc();
	// クリアバリュー設定
	void SetClearValue();

	// RTVディスクリプタヒープの作成
	HRESULT CreateRTVHeap(ComPtr<ID3D12Device> _dev);

	// DSVディスクリプタヒープの作成
	HRESULT CreateDSVHeap(ComPtr<ID3D12Device> _dev);

	// CBV,SRV兼用ディスクリプタヒープの作成
	HRESULT CreateCBVSRVHeap(ComPtr<ID3D12Device> _dev);

	// マルチパスRTV用ディスクリプタヒープの作成
	HRESULT CreateMultipassRTVHeap(ComPtr<ID3D12Device> _dev);

	// マルチパスSRV用ディスクリプタヒープの作成
	HRESULT CreateMultipassSRVHeap(ComPtr<ID3D12Device> _dev);

	// 頂点バッファーの作成
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev);

	// インデックスバッファーの作成
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev);

	// デプスバッファーの作成
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev);

	// 行列用定数バッファーの作成 WVP:World,View,Projection
	HRESULT CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev);

	// マテリアル用定数バッファーの作成。PMDMaterialInfo->MaterialForHlsl定義用。
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev);

	// マルチパスレンダリング用のバッファ作成
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_RESOURCE_DESC& mutipassResDesc);

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

	// 白黒グレー各テクスチャバッファ作成
	void CreateTextureBuffers(ComPtr<ID3D12Device> _dev);
	
	// 外部公開用
	unsigned long GetMaterialBuffSize();
	ComPtr<ID3D12DescriptorHeap> GetRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap();
	ComPtr<ID3D12DescriptorHeap> GetCBVSRVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMultipassRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMultipassSRVHeap();
	ComPtr<ID3D12Resource> GetVertBuff();
	ComPtr<ID3D12Resource> GetIdxBuff();
	ComPtr<ID3D12Resource> GetDepthBuff();
	ComPtr<ID3D12Resource> GetMatrixBuff();
	ComPtr<ID3D12Resource> GetMaterialBuff();
	ComPtr<ID3D12Resource> GetMultipassBuff();
	ComPtr<ID3D12Resource> GetMultipassBuff2();
	ComPtr<ID3D12Resource> GetWhiteTextureBuff();
	ComPtr<ID3D12Resource> GetBlackTextureBuff();
	ComPtr<ID3D12Resource> GetGrayTextureBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexReadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetsphMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetspaMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonReadBuff();
};