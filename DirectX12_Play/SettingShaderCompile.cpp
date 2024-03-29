#include <stdafx.h>
#include <SettingShaderCompile.h>

std::pair<ComPtr<ID3D10Blob>, ComPtr<ID3D10Blob>> SettingShaderCompile::SetShaderCompile
(SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	result = D3DCompileFromFile
	(
		//L"BasicVertexShader.hlsl",
		L"C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\DirectX12_Play\\BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		_vsBlob.ReleaseAndGetAddressOf()
		, setRootSignature->GetErrorBlob().GetAddressOf()
	);
	
	result = D3DCompileFromFile
	(
		//L"BasicPixelShader.hlsl",
		L"C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\DirectX12_Play\\BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		_psBlob.ReleaseAndGetAddressOf()
		, setRootSignature->GetErrorBlob().GetAddressOf()
	);
	
	//エラーチェック
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
			//return 0;
			blobs.first = nullptr;
			blobs.second = nullptr;
			return blobs;
		}
		else
		{
			std::string errstr;
			errstr.resize(setRootSignature->GetErrorBlob()->GetBufferSize());

			std::copy_n((char*)setRootSignature->GetErrorBlob()->GetBufferPointer(),
				setRootSignature->GetErrorBlob()->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
	}

	blobs.first = _vsBlob;
	blobs.second = _psBlob;
	return blobs;
}