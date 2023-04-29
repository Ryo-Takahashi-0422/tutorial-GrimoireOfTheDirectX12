#include <stdafx.h>
#include <CreateD3DX12ResourceBuffer.h>



// テクスチャ用　CPUからのアップロード用バッファ、GPUからの読み取り用バッファ、DirectX::Image生成
// @param metaData ロードしたファイルのTexmetadataオブジェクト
// @param img ロードしたファイルのImageオブジェクト
// @return CPUからのアップロード用バッファ,GPUからの読み取り用バッファ,DirectX::TexMetadata,DirectX::Image

std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> 
CreateD3DX12ResourceBuffer::LoadTextureFromFile(ComPtr<ID3D12Device> _dev, TexMetadata* metaData, Image* img, std::string& texPath)
{
	auto& utility = Utility::Instance(); ////////////////////

	std::map<std::string, std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	//メソッド内でimg等動的メモリ確保のポインタを返すことは不可能と理解した
	ComPtr<ID3D12Resource> texUploadBuff = nullptr;//テクスチャCPUアップロード用バッファー

	//テクスチャバッファー用のCPU特化型ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES texUploadHeapProp;
	texUploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	texUploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texUploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texUploadHeapProp.CreationNodeMask = 0; // 単一アダプターのため
	texUploadHeapProp.VisibleNodeMask = 0; // 単一アダプターのため

	//アップロード用
	D3D12_RESOURCE_DESC texUploadResourceDesc = {};
	texUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texUploadResourceDesc.Alignment = 0;
	texUploadResourceDesc.Width = utility.AlignmentSize(img->slicePitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;// *5;
	texUploadResourceDesc.Height = 1;
	texUploadResourceDesc.DepthOrArraySize = 1;
	texUploadResourceDesc.MipLevels = 1;
	texUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	texUploadResourceDesc.SampleDesc.Count = 1;
	texUploadResourceDesc.SampleDesc.Quality = 0;
	texUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//CPUからのアップロード用テクスチャバッファーを作成
	HRESULT result = _dev->CreateCommittedResource
	(&texUploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texUploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadタイプのヒープにおける推奨設定
		nullptr,
		IID_PPV_ARGS(texUploadBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}


	ComPtr<ID3D12Resource> texReadBuff = nullptr;//テクスチャGPU読み取り用バッファー

	//テクスチャバッファー用のGPU特化型ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES texReadHeapProp = {};
	texReadHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	texReadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texReadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texReadHeapProp.CreationNodeMask = 0; // 単一アダプターのため
	texReadHeapProp.VisibleNodeMask = 0; // 単一アダプターのため

	//GPU読み取り用
	D3D12_RESOURCE_DESC texReadResourceDesc = {};
	texReadResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData->dimension);
	texReadResourceDesc.Alignment = 0;
	texReadResourceDesc.Width = metaData->width;
	texReadResourceDesc.Height = metaData->height;
	texReadResourceDesc.DepthOrArraySize = metaData->arraySize;
	texReadResourceDesc.MipLevels = metaData->mipLevels;
	texReadResourceDesc.Format = metaData->format;
	texReadResourceDesc.SampleDesc.Count = 1;
	texReadResourceDesc.SampleDesc.Quality = 0;
	texReadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texReadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//GPUからの読み取り用テクスチャバッファーを作成
	result = _dev->CreateCommittedResource
	(&texReadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texReadResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // バッファーがCPUからのリソースコピー先であることを示す
		nullptr,
		IID_PPV_ARGS(texReadBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}

	_resourceTable[texPath] = std::forward_as_tuple(texUploadBuff.Get(), texReadBuff.Get());
	return std::forward_as_tuple(texUploadBuff.Get(), texReadBuff.Get());
}