#pragma once

class PeraSetRootSignature : public SetRootSignatureBase
{
private:
	D3D12_STATIC_SAMPLER_DESC sampler;
	CD3DX12_DESCRIPTOR_RANGE descTableRange[7] = {};
	D3D12_ROOT_PARAMETER rootParam[7] = {};

public:
	HRESULT SetRootsignatureParam(ComPtr<ID3D12Device> _dev);
	~PeraSetRootSignature();
};