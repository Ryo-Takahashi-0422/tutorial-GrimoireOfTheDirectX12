#pragma once

class SetRootSignatureBase
{
protected:

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	HRESULT result;
	ComPtr<ID3DBlob> _rootSigBlob = nullptr; // ���[�g�V�O�l�`���I�u�W�F�N�g�i�[�p
	ComPtr<ID3DBlob> _errorBlob = nullptr; // �V�F�[�_�[�֘A�G���[�i�[�p
	ComPtr<ID3D12RootSignature> _rootSignature = nullptr;

public:
	virtual HRESULT SetRootsignatureParam(ComPtr<ID3D12Device> _dev) = 0;
	~SetRootSignatureBase();
	ComPtr<ID3DBlob> GetRootSigBlob() { return _rootSigBlob; };
	ComPtr<ID3DBlob> GetErrorBlob() { return _errorBlob; };
	ComPtr<ID3D12RootSignature> GetRootSignature() { return _rootSignature; };
};