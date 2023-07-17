#include <stdafx.h>
#include <PeraSetRootSignature.h>

HRESULT PeraSetRootSignature::SetRootsignatureParam(ComPtr<ID3D12Device> _dev) {
	//●リソース初期化
	// 初期化処理1：ルートシグネチャ設定

	//サンプラー作成
	sampler = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	//サンプラーのスロット設定
	descTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;
	rootParam.DescriptorTable.pDescriptorRanges = &descTableRange;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParam;
	rootSignatureDesc.NumStaticSamplers =1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//rootSignatureDesc.NumParameters = 0;
	//rootSignatureDesc.NumStaticSamplers = 0;
	//rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	result = D3D12SerializeRootSignature //シリアル化
	(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		_rootSigBlob.ReleaseAndGetAddressOf(),
		_errorBlob.ReleaseAndGetAddressOf()
	);

	result = _dev->CreateRootSignature
	(
		0,
		_rootSigBlob->GetBufferPointer(),
		_rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(_rootSignature.ReleaseAndGetAddressOf())
	);

	_rootSigBlob->Release();

	return S_OK;
}

PeraSetRootSignature::~PeraSetRootSignature()
{
	delete this;
}
