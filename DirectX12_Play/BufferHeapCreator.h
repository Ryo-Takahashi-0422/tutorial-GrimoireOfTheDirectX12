#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; // �����_�[�^�[�Q�b�g�r���[�p�f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // �[�x�X�e���V���r���[�p�f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC cbvsrvHeapDesc = {}; // �s��p�f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC mutipassRTVHeapDesc = {}; // �}���`�p�XRTV�p�f�B�X�N���v�^�q�[�v
	D3D12_DESCRIPTOR_HEAP_DESC mutipassSRVHeapDesc = {}; // �}���`�p�XSRV�p�f�B�X�N���v�^�q�[�v

	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr; // ���\�[�X�^�[�Q�b�g�r���[�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr; // �[�x�X�e���V���r���[�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> cbvsrvHeap = nullptr; // �s��CBV, SRV�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> multipassRTVHeap = nullptr; // �}���`�p�X�����_�[�^�[�Q�b�g�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> multipassSRVHeap = nullptr; // �}���`�p�X�e�N�X�`���p�f�B�X�N���v�^�q�[�v

	D3D12_HEAP_PROPERTIES vertexHeapProps = {}; // ���_�A���_�C���f�b�N�X�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProps = {}; // �[�x�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES wvpHeapProp = {}; // �s��p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES materialHeapProp = {};  // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���
	D3D12_HEAP_PROPERTIES mutipassHeapProp = {}; // �}���`�p�X�p�q�[�v�v���p�e�B

	D3D12_RESOURCE_DESC vertresDesc; // ���_�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC indicesDesc; // ���_�C���f�b�N�X�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC depthResDesc = {}; // �[�x���\�[�X�ڍ�
	D3D12_RESOURCE_DESC wvpResdesc = {}; // �s��p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC materialBuffResDesc = {}; // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���
	// ����߽�ޯ̧�p�̃I�u�W�F�N�g��AppD3DX12���������Ă��邽�ߕێ����Ȃ�

	ComPtr<ID3D12Resource> vertBuff = nullptr; // ���_�p�o�b�t�@
	ComPtr<ID3D12Resource> idxBuff = nullptr; // ���_�C���f�b�N�X�p�o�b�t�@
	ComPtr<ID3D12Resource> depthBuff = nullptr; // �f�v�X�o�b�t�@�[
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // �s��p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> materialBuff = nullptr; // �}�e���A���p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> multipassBuff = nullptr; // �}���`�p�X�����_�����O�p�������ݐ�o�b�t�@�[
	ComPtr<ID3D12Resource> whiteTextureBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> blackTextureBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> grayTextureBuff = nullptr; // �O���[�e�N�X�`���p�o�b�t�@�[

	std::vector<ComPtr<ID3D12Resource>> texUploadBuff;//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> texReadBuff;//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff;//sph�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff;//spa�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff;//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff;//�g�D�[���p���[�h�o�b�t�@�[

	D3D12_CLEAR_VALUE depthClearValue = {}; // �[�x�N���A�o�����[

	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string toonFilePath; // �g�D�[���e�N�X�`���ۑ��t�H���_�̃p�X
	HRESULT result;

	unsigned long materialBuffSize;

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	// RTV�p�f�B�X�N���v�^�q�[�v�ݒ�
	void SetRTVHeapDesc();
	// DSV�p�f�B�X�N���v�^�q�[�v�ݒ�
	void SetDSVHeapDesc();
	// CBV,SRV�p�f�B�X�N���v�^�q�[�v�ݒ�(�s��CBV�A��ر�CBV�Aø���SRV���Z�b�g)
	void SetCBVSRVHeapDesc();
	// �}���`�p�XRTV�p�f�B�X�N���v�^�q�[�v�ݒ�
	void SetMutipassRTVHeapDesc();
	// �}���`�p�XSRV�p�f�B�X�N���v�^�q�[�v�ݒ�
	void SetMutipassSRVHeapDesc();
	// ���_�E�C���f�b�N�X�p�q�[�v�v���p�e�B�ݒ�
	void SetVertexHeapProp();
	// �[�x�p�q�[�v�v���p�e�B�ݒ�
	void SetDepthHeapProp();
	// �[�x�p���\�[�X�ڍאݒ�
	void SetDepthResourceDesc();
	// �N���A�o�����[�ݒ�
	void SetClearValue();

	// RTV�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT CreateRTVHeap(ComPtr<ID3D12Device> _dev);

	// DSV�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT CreateDSVHeap(ComPtr<ID3D12Device> _dev);

	// CBV,SRV���p�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT CreateCBVSRVHeap(ComPtr<ID3D12Device> _dev);

	// �}���`�p�XRTV�p�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT CreateMultipassRTVHeap(ComPtr<ID3D12Device> _dev);

	// �}���`�p�XSRV�p�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT CreateMultipassSRVHeap(ComPtr<ID3D12Device> _dev);

	// ���_�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev);

	// �C���f�b�N�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev);

	// �f�v�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev);

	// �s��p�萔�o�b�t�@�[�̍쐬 WVP:World,View,Projection
	HRESULT CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev);

	// �}�e���A���p�萔�o�b�t�@�[�̍쐬�BPMDMaterialInfo->MaterialForHlsl��`�p�B
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev);

	// �}���`�p�X�����_�����O�p�̃o�b�t�@�쐬
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_RESOURCE_DESC& mutipassResDesc);

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

	// �����O���[�e�e�N�X�`���o�b�t�@�쐬
	void CreateTextureBuffers(ComPtr<ID3D12Device> _dev);
	
	// �O�����J�p
	unsigned long GetMaterialBuffSize();
	ComPtr<ID3D12DescriptorHeap> GetRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap();
	ComPtr<ID3D12DescriptorHeap> GetCBVSRVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMultipassRTVHeap();
	ComPtr<ID3D12DescriptorHeap> GetMultipassSRVHeap();
	ComPtr<ID3D12Resource> GetVertBuff();
	ComPtr<ID3D12Resource> GetIdxBuff();
	ComPtr<ID3D12Resource> GetDepthBuff();
	ComPtr<ID3D12Resource> GetMatrixBuff();
	ComPtr<ID3D12Resource> GetMaterialBuff();
	ComPtr<ID3D12Resource> GetMultipassBuff();
	ComPtr<ID3D12Resource> GetWhiteTextureBuff();
	ComPtr<ID3D12Resource> GetBlackTextureBuff();
	ComPtr<ID3D12Resource> GetGrayTextureBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetTexReadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetsphMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetspaMappedBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonUploadBuff();
	std::vector<ComPtr<ID3D12Resource>> GetToonReadBuff();
};