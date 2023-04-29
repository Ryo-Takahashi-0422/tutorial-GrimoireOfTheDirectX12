#pragma once
//using namespace Microsoft::WRL;
//using namespace DirectX;

class CreateD3DX12ResourceBuffer 
{
private:




public:
	std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>  LoadTextureFromFile
	(
		ComPtr<ID3D12Device> _dev,
		TexMetadata* metaData,
		Image* img,
		std::string& texPath
	);
};