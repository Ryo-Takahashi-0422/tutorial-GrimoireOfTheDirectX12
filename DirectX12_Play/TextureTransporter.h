#pragma once

class TextureTransporter
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	// PMDテクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> pmdSource;
	std::vector<D3D12_TEXTURE_COPY_LOCATION> pmdDestination;
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc;

public:
	TextureTransporter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);

	// 任意のテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	void TransportTexture(ComPtr<ID3D12GraphicsCommandList> _cmdList,
		ComPtr<ID3D12CommandAllocator> _cmdAllocator,
		ComPtr<ID3D12CommandQueue> _cmdQueue,
		std::vector<DirectX::TexMetadata*> metaData,
		std::vector<DirectX::Image*> img,
		ComPtr<ID3D12Fence> _fence,
		UINT64& _fenceVal);

	// PMDテクスチャをマテリアル数分まとめてGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	void TransportPMDMaterialTexture(ComPtr<ID3D12GraphicsCommandList> _cmdList,
		ComPtr<ID3D12CommandAllocator> _cmdAllocator,
		ComPtr<ID3D12CommandQueue> _cmdQueue,
		std::vector<DirectX::TexMetadata*> metaData,
		std::vector<DirectX::Image*> img,
		ComPtr<ID3D12Fence> _fence,
		UINT64& _fenceVal,
		std::vector<ComPtr<ID3D12Resource>> uploadBuff,
		std::vector<ComPtr<ID3D12Resource>> readBuff);
};