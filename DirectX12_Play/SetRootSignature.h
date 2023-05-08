#pragma once

class SetRootSignature 
{
private:
	CD3DX12_STATIC_SAMPLER_DESC stSamplerDesc[2] = {};
	CD3DX12_DESCRIPTOR_RANGE descTableRange[3] = {};
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	HRESULT result;
	ComPtr<ID3DBlob> _rootSigBlob = nullptr; // ���[�g�V�O�l�`���I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> _errorBlob = nullptr; // �V�F�[�_�[�֘A�G���[�i�[�p
	ComPtr<ID3D12RootSignature> _rootSignature = nullptr;

public:
	HRESULT SetRootsignatureParam(ComPtr<ID3D12Device> _dev);
	ComPtr<ID3DBlob> GetRootSigBlob() { return _rootSigBlob; };
	ComPtr<ID3DBlob> GetErrorBlob() { return _errorBlob; };
	ComPtr<ID3D12RootSignature> GetRootSignature() { return _rootSignature; };
};