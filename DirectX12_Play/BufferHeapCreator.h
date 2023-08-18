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
	//ComPtr<ID3D12DescriptorHeap> imguiPostSettingHeap = nullptr; // imgui PostSetting�\���̗p�f�B�X�N���v�^�q�[�v

	D3D12_HEAP_PROPERTIES vertexHeapProps = {}; // ���_�A���_�C���f�b�N�X�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProps = {}; // �[�x�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES lightMapHeapProps = {}; // ���C�g�}�b�v�o�b�t�@�[�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES wvpHeapProp = {}; // �s��p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES materialHeapProp = {};  // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���
	D3D12_HEAP_PROPERTIES mutipassHeapProp = {}; // �}���`�p�X�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES gaussianHeapProp = {}; // �K�E�X�ڂ����p�q�[�v�v���p�e�B

	D3D12_RESOURCE_DESC vertresDesc; // ���_�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC indicesDesc; // ���_�C���f�b�N�X�p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC depthResDesc = {}; // �[�x���\�[�X�ڍ�
	D3D12_RESOURCE_DESC lightMapResDesc = {}; // ���C�g�}�b�v�p�o�b�t�@�[�ڍ�
	D3D12_RESOURCE_DESC wvpResdesc = {}; // �s��p���\�[�X�ڍ�
	D3D12_RESOURCE_DESC materialBuffResDesc = {}; // PMDMaterialInfo->MaterialForHlsl��`�ɍ��킹�Ă���
	// ����߽�ޯ̧�p�̃I�u�W�F�N�g��AppD3DX12���������Ă��邽�ߕێ����Ȃ�
	D3D12_RESOURCE_DESC gaussianBuffResDesc = {}; // �K�E�X�ڂ����p

	ComPtr<ID3D12Resource> vertBuff = nullptr; // ���_�p�o�b�t�@
	ComPtr<ID3D12Resource> idxBuff = nullptr; // ���_�C���f�b�N�X�p�o�b�t�@
	ComPtr<ID3D12Resource> depthBuff = nullptr; // �f�v�X�o�b�t�@�[
	ComPtr<ID3D12Resource> depthBuff2 = nullptr; // �f�v�X�o�b�t�@�[
	ComPtr<ID3D12Resource> lightMapBuff = nullptr; // ���C�g�}�b�v�o�b�t�@�[
	ComPtr<ID3D12Resource> matrixBuff = nullptr; // �s��p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> matrixBuff4Multipass = nullptr; // ����߽�p�s��p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> materialBuff = nullptr; // �}�e���A���p�萔�o�b�t�@�[
	ComPtr<ID3D12Resource> multipassBuff = nullptr; // �}���`�p�X�����_�����O�p�������ݐ�o�b�t�@�[
	ComPtr<ID3D12Resource> multipassBuff2 = nullptr; // �}���`�p�X�����_�����O�p�������ݐ�o�b�t�@�[����2
	ComPtr<ID3D12Resource> multipassBuff3 = nullptr; // �}���`�p�X�����_�����O�p�������ݐ�o�b�t�@�[����3
	ComPtr<ID3D12Resource> whiteTextureBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> blackTextureBuff = nullptr; // ���e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> grayTextureBuff = nullptr; // �O���[�e�N�X�`���p�o�b�t�@�[
	ComPtr<ID3D12Resource> gaussianBuff = nullptr; // �K�E�X�ڂ����p�o�b�t�@�[
	std::array<ComPtr<ID3D12Resource>, 3> _bloomBuff; // �u���[���p�o�b�t�@�[+shrinkedModel
	ComPtr<ID3D12Resource> aoBuff = nullptr; // buffer for AO
	ComPtr<ID3D12Resource> imguiBuff = nullptr; // buffer for imgui rendering
	ComPtr<ID3D12Resource> imguiPostSettingBuff = nullptr; // buffer for imgui PostSetting

	std::vector<ComPtr<ID3D12Resource>> normalMapUploadBuff; // �m�[�}���}�b�v�p�A�b�v���[�h�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> normalMapReadBuff; // �m�[�}���}�b�v�p���[�h�o�b�t�@�[
	// PMD���f���p�̃����o�[
	std::vector<ComPtr<ID3D12Resource>> pmdTexUploadBuff; // PMD���f���̃e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> pmdTexReadBuff; // PMD���f���̃e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> sphMappedBuff; // sph�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> spaMappedBuff; // spa�p�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonUploadBuff; // �g�D�[���p�A�b�v���[�h�o�b�t�@�[
	std::vector<ComPtr<ID3D12Resource>> toonReadBuff; // �g�D�[���p���[�h�o�b�t�@�[

	D3D12_CLEAR_VALUE depthClearValue = {}; // �[�x�N���A�o�����[

	std::string textureFolader = "texture";
	std::vector<DirectX::TexMetadata*> normalMapMetaData;
	std::vector<DirectX::Image*> normalMapImg;
	struct _stat s = {};
	ScratchImage scratchImg = {};
	ScratchImage normalMapScratchImg = {};
	ScratchImage toonScratchImg = {};
	std::string filePath; // �e�N�X�`���ۑ��t�H���_�̃p�X
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
	HRESULT CreateBufferOfDepthAndLightMap(ComPtr<ID3D12Device> _dev);

	// �s��p�萔�o�b�t�@�[�̍쐬 WVP:World,View,Projection
	HRESULT CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev);

	// �}�e���A���p�萔�o�b�t�@�[�̍쐬�BPMDMaterialInfo->MaterialForHlsl��`�p�B
	HRESULT CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev);

	// �}���`�p�X�����_�����O�p�̃o�b�t�@�쐬
	HRESULT CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_RESOURCE_DESC& mutipassResDesc);

	// �K�E�X�ڂ����p�萔�o�b�t�@�[�̍쐬�B
	HRESULT CreateConstBufferOfGaussian(ComPtr<ID3D12Device> _dev, std::vector<float> weights);

	// �C�Ӄt�@�C���`���̃m�[�}���}�b�v�pCPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	void CreateUploadAndReadBuff4Normalmap(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::string fileType,
		unsigned int texNum);

	// PMD���f���̃e�N�X�`���pCPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	void CreateUploadAndReadBuff4PmdTexture(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& metaData,
		std::vector<DirectX::Image*>& img);

	// PMD���f����Toon�e�N�X�`���pCPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	void CreateToonUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
		std::string strModelPath,
		std::vector<DirectX::TexMetadata*>& toonMetaData,
		std::vector<DirectX::Image*>& toonImg);

	// �����O���[�e�e�N�X�`���o�b�t�@�쐬
	void CreateTextureBuffers(ComPtr<ID3D12Device> _dev);

	// imgui�p���ި������˰�߁Aؿ�����쐬����
	HRESULT CreateBuff4Imgui(ComPtr<ID3D12Device> _dev, size_t sizeofPostSetting);
	
	// �O�����J�p
	unsigned long GetMaterialBuffSize() { return materialBuffSize; };
	ComPtr<ID3D12DescriptorHeap> GetRTVHeap() { return rtvHeaps; };
	ComPtr<ID3D12DescriptorHeap> GetDSVHeap() { return dsvHeap; };
	ComPtr<ID3D12DescriptorHeap> GetCBVSRVHeap() { return cbvsrvHeap; };
	ComPtr<ID3D12DescriptorHeap> GetMultipassRTVHeap() { return multipassRTVHeap; };
	ComPtr<ID3D12DescriptorHeap> GetMultipassSRVHeap() { return multipassSRVHeap; };
	//ComPtr<ID3D12DescriptorHeap> GetImguiPostSettingHeap() { return imguiPsotSettingHeap; };

	ComPtr<ID3D12Resource> GetVertBuff() { return vertBuff; };
	ComPtr<ID3D12Resource> GetIdxBuff() { return idxBuff; };
	ComPtr<ID3D12Resource> GetDepthBuff() { return depthBuff; };
	ComPtr<ID3D12Resource> GetDepthBuff2() { return depthBuff2; };
	ComPtr<ID3D12Resource> GetLightMapBuff() { return lightMapBuff; };
	ComPtr<ID3D12Resource> GetMatrixBuff() { return matrixBuff; };
	ComPtr<ID3D12Resource> GetMatrixBuff4Multipass() { return matrixBuff4Multipass; };
	ComPtr<ID3D12Resource> GetMaterialBuff() { return materialBuff; };
	ComPtr<ID3D12Resource> GetMultipassBuff() { return multipassBuff; };
	ComPtr<ID3D12Resource> GetMultipassBuff2() { return multipassBuff2; };
	ComPtr<ID3D12Resource> GetMultipassBuff3() { return multipassBuff3; };
	ComPtr<ID3D12Resource> GetWhiteTextureBuff() { return whiteTextureBuff; };
	ComPtr<ID3D12Resource> GetBlackTextureBuff() { return blackTextureBuff; };
	ComPtr<ID3D12Resource> GetGrayTextureBuff() { return grayTextureBuff; };
	ComPtr<ID3D12Resource> GetGaussianBuff() { return gaussianBuff; };
	ComPtr<ID3D12Resource> GetAOBuff() { return aoBuff; };
	ComPtr<ID3D12Resource> GetImguiBuff() { return imguiBuff; };
	ComPtr<ID3D12Resource> GetImguiPostSettingBuff() { return imguiPostSettingBuff; };

	std::vector<ComPtr<ID3D12Resource>> GetNormalMapUploadBuff() { return normalMapUploadBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetNormalMapReadBuff() { return normalMapReadBuff; };
	// �ȉ���PMD�t�@�C���p
	std::vector<ComPtr<ID3D12Resource>> GetPMDTexUploadBuff() { return pmdTexUploadBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetPMDTexReadBuff() { return pmdTexReadBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetsphMappedBuff() { return sphMappedBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetspaMappedBuff() { return spaMappedBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetToonUploadBuff() { return toonUploadBuff; };
	std::vector<ComPtr<ID3D12Resource>> GetToonReadBuff() { return toonReadBuff; };
	std::array<ComPtr<ID3D12Resource>, 3> GetBloomBuff() { return _bloomBuff; };
	
	
	// texture���Q
	std::vector<DirectX::TexMetadata*> GetNormalMapMetadata() { return normalMapMetaData; };
	std::vector<DirectX::Image*> GetNormalMapImg() { return normalMapImg; };
};