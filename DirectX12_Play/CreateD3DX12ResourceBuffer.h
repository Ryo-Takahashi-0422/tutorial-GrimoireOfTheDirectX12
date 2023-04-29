#pragma once
//using namespace Microsoft::WRL;
//using namespace DirectX;

class CreateD3DX12ResourceBuffer 
{
private:


public:

	// �e�N�X�`���p�@CPU����̃A�b�v���[�h�p�o�b�t�@�AGPU����̓ǂݎ��p�o�b�t�@�ADirectX::Image����
	// @param metaData ���[�h�����t�@�C����Texmetadata�I�u�W�F�N�g
	// @param img ���[�h�����t�@�C����Image�I�u�W�F�N�g
	// @return CPU����̃A�b�v���[�h�p�o�b�t�@,GPU����̓ǂݎ��p�o�b�t�@,DirectX::TexMetadata,DirectX::Image
	static std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>  LoadTextureFromFile
	(
		ComPtr<ID3D12Device> _dev,
		TexMetadata* metaData,
		Image* img,
		std::string& texPath
	);

	static ComPtr<ID3D12Resource> CreateMappedSphSpaTexResource(ComPtr<ID3D12Device> _dev, TexMetadata* metaData, Image* img, std::string texPath);
	static ComPtr<ID3D12Resource> CreateColorTexture(ComPtr<ID3D12Device> _dev, const int param);
	static ComPtr<ID3D12Resource> CreateGrayGradationTexture(ComPtr<ID3D12Device> _dev);
};