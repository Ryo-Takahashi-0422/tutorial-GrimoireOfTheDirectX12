#include <stdafx.h>
#include <SetRootSignature.h>

HRESULT SetRootSignature::SetRootsignatureParam(ComPtr<ID3D12Device> _dev) {
	//●リソース初期化
	// 初期化処理1：ルートシグネチャ設定

	//サンプラー作成
	stSamplerDesc[0].Init(0);
	stSamplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC,
	D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	//ディスクリプタテーブルのスロット設定
	descTableRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // martix
	descTableRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // material
	descTableRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // colortex, graytex, spa, sph
	descTableRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // ライトマップ用

	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1; // デプス用
	rootParam[0].DescriptorTable.pDescriptorRanges = descTableRange;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2; // マテリアル(CBV)とテクスチャ(SRV)の1セット5個で使う
	rootParam[1].DescriptorTable.pDescriptorRanges = &descTableRange[1]; // ここからNumDescriptorRange分、つまり[1]と[2]が該当する。
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1; // ライトマップ用
	rootParam[2].DescriptorTable.pDescriptorRanges = &descTableRange[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootSignatureDesc.NumParameters = 3;
	rootSignatureDesc.pParameters = rootParam;
	rootSignatureDesc.NumStaticSamplers = 2;
	rootSignatureDesc.pStaticSamplers = stSamplerDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	result = D3D12SerializeRootSignature //シリアル化
	(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		_rootSigBlob.ReleaseAndGetAddressOf(),
		_errorBlob.GetAddressOf()
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

SetRootSignature::~SetRootSignature()
{
	delete this;
}
