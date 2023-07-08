#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> matrixHeap = nullptr; // �s��CBV, SRV�f�B�X�N���v�^�q�[�v

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

	ComPtr<ID3D12Resource> _peraResource; // �}���`�p�X�����_�����O�p�������ݐ惊�\�[�X
	ComPtr<ID3D12DescriptorHeap> _peraRTVHeap; // �����_�[�^�[�Q�b�g�p
	ComPtr<ID3D12DescriptorHeap> _peraSRVHeap; // �e�N�X�`���p

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	void SetRTVHeapDesc(D3D12_DESCRIPTOR_HEAP_DESC* rtvHeapDesc); // RTV�p�q�[�v�ڍאݒ�
	void SetDSVHeapDesc();//D3D12_DESCRIPTOR_HEAP_DESC* dsvHeapDesc); // DSV�p�q�[�v�ڍאݒ�
	void SetMatrixHeapDesc(D3D12_DESCRIPTOR_HEAP_DESC* matrixHeapDesc); // �s��p�q�[�v�ڍאݒ�
	void SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps); // ���_�E�C���f�b�N�X�p�q�[�v�v���p�e�B�ݒ�
	void SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps); // �[�x�p�q�[�v�v���p�e�B�ݒ�
	void SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc); // �[�x�p���\�[�X�ڍאݒ�
	void SetClearValue(D3D12_CLEAR_VALUE* depthClearValue); // �N���A�o�����[�ݒ�

	// RTV�q�[�v�̍쐬
	HRESULT CreateRTVHeap(ComPtr<ID3D12Device> _dev, D3D12_DESCRIPTOR_HEAP_DESC& rtvHeapDesc);

	// DSV�q�[�v�̍쐬
	HRESULT CreateDSVHeap(ComPtr<ID3D12Device> _dev);//, D3D12_DESCRIPTOR_HEAP_DESC& dsvHeapDesc);

	// �s��p�q�[�v�̍쐬
	HRESULT CreateMatrixHeap(ComPtr<ID3D12Device> _dev, D3D12_DESCRIPTOR_HEAP_DESC& matrixHeapDesc);

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

	// �}���`�p�X�����_�����O�p�̃o�b�t�@�쐬
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& mutipassHeapProp,
		D3D12_RESOURCE_DESC& mutipassResDesc, D3D12_CLEAR_VALUE clearValue);

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
	
	ComPtr<ID3D12DescriptorHeap> GetRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMatrixHeap();
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