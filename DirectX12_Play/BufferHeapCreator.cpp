#include <stdafx.h>
#include <BufferHeapCreator.h>

BufferHeapCreator::BufferHeapCreator(PrepareRenderingWindow* _prepareRenderingWindow)
{
	prepareRenderingWindow = new PrepareRenderingWindow;
	prepareRenderingWindow = _prepareRenderingWindow;
}

void BufferHeapCreator::SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps)
{
	heapProps->Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps->MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps->CreationNodeMask = 0;
	heapProps->VisibleNodeMask = 0;
}

void BufferHeapCreator::SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps)
{
	depthHeapProps->Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProps->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProps->MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void BufferHeapCreator::SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc)
{
	depthResDesc->Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc->Width = prepareRenderingWindow->GetWindowWidth();
	depthResDesc->Height = prepareRenderingWindow->GetWindowHeight();
	depthResDesc->DepthOrArraySize = 1;
	depthResDesc->Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用
	depthResDesc->SampleDesc.Count = 1; // 1pixce/1つのサンプル
	depthResDesc->Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
}

void BufferHeapCreator::SetClearValue(D3D12_CLEAR_VALUE* depthClearValue)
{
	depthClearValue->Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue->DepthStencil.Depth = 1.0f; // 深さ1.0(最大値)でクリア
}

HRESULT BufferHeapCreator::CreateBufferOfVertex(ComPtr<ID3D12Device> _dev, 
	D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& vertresDesc)
{
	return _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの状態。GPUからして読み取り用
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfIndex(ComPtr<ID3D12Device> _dev,
	D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& indicesDesc)
{
	return _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfDepth(ComPtr<ID3D12Device> _dev,
	D3D12_HEAP_PROPERTIES& depthHeapProps, D3D12_RESOURCE_DESC& depthResDesc)
{
	return _dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(depthBuff.ReleaseAndGetAddressOf())
	);
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetVertBuff()
{
	return vertBuff;
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetIdxBuff()
{
	return idxBuff;
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetDepthBuff()
{
	return depthBuff;
}