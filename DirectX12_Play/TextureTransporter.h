#pragma once

class TextureTransporter
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	// テクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> src;
	std::vector<D3D12_TEXTURE_COPY_LOCATION> dst;
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc;

	// トゥーンテクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonSrc;
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonDst;
	std::vector<D3D12_RESOURCE_BARRIER> toonBarriierDesc;

public:
	TextureTransporter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);

	// テクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	void TransportTexture(ComPtr<ID3D12GraphicsCommandList> _cmdList,
		ComPtr<ID3D12CommandAllocator> _cmdAllocator,
		ComPtr<ID3D12CommandQueue> _cmdQueue,
		std::vector<DirectX::TexMetadata*> metaData,
		std::vector<DirectX::Image*> img,
		ComPtr<ID3D12Fence> _fence,
		UINT64& _fenceVal);

	// トゥーンテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	void TransportToonTexture(ComPtr<ID3D12GraphicsCommandList> _cmdList,
		ComPtr<ID3D12CommandAllocator> _cmdAllocator,
		ComPtr<ID3D12CommandQueue> _cmdQueue,
		std::vector<DirectX::TexMetadata*> toonMetaData,
		std::vector<DirectX::Image*> toonImg,
		ComPtr<ID3D12Fence> _fence,
		UINT64& _fenceVal);
};