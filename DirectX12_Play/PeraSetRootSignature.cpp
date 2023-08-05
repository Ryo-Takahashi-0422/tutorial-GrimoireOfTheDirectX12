#include <stdafx.h>
#include <PeraSetRootSignature.h>

HRESULT PeraSetRootSignature::SetRootsignatureParam(ComPtr<ID3D12Device> _dev) {

	//サンプラー作成
	sampler = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	//サンプラーのスロット設定
	descTableRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // multipassBuff1用
	descTableRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // multipassBuff2用
	descTableRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // effect用
	descTableRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // normalMapReadBuff用
	descTableRange[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // depthBuff4DepthMap用
	descTableRange[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // lightmap用
	descTableRange[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // シーン行列用
	descTableRange[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5); // マルチターゲット法線用
	descTableRange[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6); // bloom

	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descTableRange[0];
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descTableRange[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descTableRange[2];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].DescriptorTable.pDescriptorRanges = &descTableRange[3];
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[4].DescriptorTable.pDescriptorRanges = &descTableRange[4];
	rootParam[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[5].DescriptorTable.pDescriptorRanges = &descTableRange[5];
	rootParam[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[6].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[6].DescriptorTable.pDescriptorRanges = &descTableRange[6];
	rootParam[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[7].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[7].DescriptorTable.pDescriptorRanges = &descTableRange[7];
	rootParam[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[8].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[8].DescriptorTable.pDescriptorRanges = &descTableRange[8];
	rootParam[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignatureDesc.NumParameters = 9;
	rootSignatureDesc.pParameters = rootParam;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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

//PeraSetRootSignature::~PeraSetRootSignature()
//{
//}
