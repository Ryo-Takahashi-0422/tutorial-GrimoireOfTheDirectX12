#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;
	ComPtr<ID3D12Resource> vertBuff = nullptr;
	ComPtr<ID3D12Resource> idxBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	ComPtr<ID3D12Resource> depthBuff = nullptr; // �f�v�X�o�b�t�@�[
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // �s��p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> materialBuff = nullptr; // �}�e���A���p�萔�o�b�t�@�[

	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//�g�D�[���p���[�h�o�b�t�@�[
	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string toonFilePath;
	HRESULT result;

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	void SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps);
	void SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps);
	void SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc);
	void SetClearValue(D3D12_CLEAR_VALUE* depthClearValue);

	// ���_�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& vertresDesc);

	// �C���f�b�N�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& indicesDesc);

	// �f�v�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& depthResDesc);

	// �s��p�萔�o�b�t�@�[�̍쐬
	HRESULT CreateConstBufferOfMatrix(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& constBuffProp, D3D12_RESOURCE_DESC& constBuffResdesc);

	// �}�e���A���p�萔�o�b�t�@�[�̍쐬
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& materialHeapProp, D3D12_RESOURCE_DESC& materialBuffResDesc);

	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	void CreateUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& metaData,
		std::vector<DirectX::Image*>& img);

	// Toon�e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	void CreateToonUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& toonMetaData,
		std::vector<DirectX::Image*>& toonImg);

	ComPtr<ID3D12Resource> GetVertBuff();
	ComPtr<ID3D12Resource> GetIdxBuff();
	ComPtr<ID3D12Resource> GetDepthBuff();
	ComPtr<ID3D12Resource> GetMatrixBuff();
	ComPtr<ID3D12Resource> GetMaterialBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexReadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetsphMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetspaMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonReadBuff();
};