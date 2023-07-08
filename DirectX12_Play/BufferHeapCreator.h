#pragma once

class BufferHeapCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	PrepareRenderingWindow* prepareRenderingWindow = nullptr;
	TextureLoader* textureLoader = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; // �����_�[�^�[�Q�b�g�r���[�p�q�[�v�ڍ�
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // �[�x�X�e���V���r���[�p�q�[�v�ڍ�
	D3D12_DESCRIPTOR_HEAP_DESC matrixHeapDesc = {}; // �s��p�q�[�v�ڍ�

	D3D12_RESOURCE_DESC depthResDesc = {}; // �[�x���\�[�X�ڍ�
	D3D12_CLEAR_VALUE depthClearValue = {}; // �[�x�N���A�o�����[

	//ID3D12Resource�I�u�W�F�N�g�̓����p�����[�^�ݒ�
	D3D12_RESOURCE_DESC vertresDesc; // ���_�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC indicesDesc; // ���_�C���f�b�N�X�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC wvpResdesc = {}; // �s��p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC materialBuffResDesc = {}; // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���

	D3D12_HEAP_PROPERTIES vertexHeapProps = {}; // ���_�A���_�C���f�b�N�X�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProps = {}; // �[�x�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES wvpHeapProp = {}; // �s��p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES materialHeapProp = {};  // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���

	ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr; // ���\�[�X�^�[�Q�b�g�r���[�p�q�[�v
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr; // �[�x�X�e���V���r���[�p�q�[�v
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
	ComPtr<ID3D12Resource> whiteBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> BlackBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> grayTexBuff = nullptr; // �O���[�e�N�X�`���p�o�b�t�@�[

	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string toonFilePath; // �g�D�[���e�N�X�`���ۑ��t�H���_�̃p�X
	HRESULT result;

	ComPtr<ID3D12Resource> _peraResource; // �}���`�p�X�����_�����O�p�������ݐ惊�\�[�X
	ComPtr<ID3D12DescriptorHeap> _peraRTVHeap; // �����_�[�^�[�Q�b�g�p
	ComPtr<ID3D12DescriptorHeap> _peraSRVHeap; // �e�N�X�`���p

	unsigned long materialBuffSize;

public:
	BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo, PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader);
	void SetRTVHeapDesc(); // RTV�p�q�[�v�ڍאݒ�
	void SetDSVHeapDesc(); // DSV�p�q�[�v�ڍאݒ�
	void SetMatrixHeapDesc(); // �s��p�q�[�v�ڍאݒ�
	void SetVertexAndIndexHeapProp(); // ���_�E�C���f�b�N�X�p�q�[�v�v���p�e�B�ݒ�
	void SetDepthHeapProp(); // �[�x�p�q�[�v�v���p�e�B�ݒ�
	void SetDepthResourceDesc(); // �[�x�p���\�[�X�ڍאݒ�
	void SetClearValue(); // �N���A�o�����[�ݒ�

	// RTV�q�[�v�̍쐬
	HRESULT CreateRTVHeap(ComPtr<ID3D12Device> _dev);

	// DSV�q�[�v�̍쐬
	HRESULT CreateDSVHeap(ComPtr<ID3D12Device> _dev);

	// �s��p�q�[�v�̍쐬
	HRESULT CreateMatrixHeap(ComPtr<ID3D12Device> _dev);

	// ���_�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfVertex(ComPtr<ID3D12Device> _dev);

	// �C���f�b�N�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfIndex(ComPtr<ID3D12Device> _dev);

	// �f�v�X�o�b�t�@�[�̍쐬
	HRESULT CreateBufferOfDepth(ComPtr<ID3D12Device> _dev);

	// �s��p�萔�o�b�t�@�[�̍쐬
	HRESULT CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev);

	// �}�e���A���p�萔�o�b�t�@�[�̍쐬�BPMDMaterialInfo->MaterialForHlsl��`�p�B
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev/*, D3D12_HEAP_PROPERTIES& materialHeapProp, D3D12_RESOURCE_DESC& materialBuffResDesc*/);

	// �}���`�p�X�����_�����O�p�̃o�b�t�@�쐬
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_HEAP_PROPERTIES& mutipassHeapProp, D3D12_RESOURCE_DESC& mutipassResDesc);

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
	
	// �O�����J�p
	unsigned long GetMaterialBuffSize();
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