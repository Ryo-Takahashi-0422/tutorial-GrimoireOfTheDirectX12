#pragma once

class BufferHeapCreator
{
private:
	ID3D12Device* dev = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	ComPtr<ID3D12Resource> vertBuff = nullptr;
	ComPtr<ID3D12Resource> idxBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	ComPtr<ID3D12Resource> depthBuff = nullptr; // �f�v�X�o�b�t�@�[

public:
	BufferHeapCreator(PrepareRenderingWindow* _prepareRenderingWindow);
	void SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps);
	void SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps);
	void SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc);
	void SetClearValue(D3D12_CLEAR_VALUE* depthClearValue);
	//���_�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& vertresDesc);

	//�C���f�b�N�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& indicesDesc);

	//�f�v�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& depthResDesc);

	ComPtr<ID3D12Resource> GetVertBuff();
	ComPtr<ID3D12Resource> GetIdxBuff();
	ComPtr<ID3D12Resource> GetDepthBuff();
};