#pragma once

class PeraSetRootSignature : public SetRootSignatureBase
{
private:
	CD3DX12_STATIC_SAMPLER_DESC stSamplerDesc[2] = {};
	CD3DX12_DESCRIPTOR_RANGE descTableRange[3] = {};
	D3D12_ROOT_PARAMETER rootParam[2] = {};

public:
	HRESULT SetRootsignatureParam(ComPtr<ID3D12Device> _dev);
	~PeraSetRootSignature();
};