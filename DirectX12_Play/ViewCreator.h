#pragma once

class ViewCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vbView = {}; // Vertex�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {}; // (Vertex)Index�r���[
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {}; // �s��p�̒萔�r���[�ڍ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {}; // �f�v�X�X�e���V���r���[�ڍ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc4MaterialAndTextureAndSph = {}; // �}�e���A�����A�e�N�X�`���Asph�p�̒萔�r���[�ڍ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc4MaterialAndTextureAndSph = {}; // �V�F�[�_�[���\�[�X�r���[�ڍ�
	D3D12_RENDER_TARGET_VIEW_DESC multipassRTVDesc = {}; // �}���`�p�X�p�����_�[�^�[�Q�b�g�r���[�ڍ�
	D3D12_SHADER_RESOURCE_VIEW_DESC multipassSRVDesc = {}; // �}���`�p�X�p�����_�[�^�[�Q�b�g�r���[�ڍ�

	D3D12_CPU_DESCRIPTOR_HANDLE basicDescHeapHandle;

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	void SetCBVDesc4Matrix();
	void SetDSVDesc();
	void SetCBVDesc4MaterialAndTextureAndSph();
	void SetSRVDesc4MaterialAndTextureAndSph();
	void SetRTVDesc4Multipass();
	void SetSRVDesc4Multipass();

public:
	ViewCreator(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);
	void CreateCBV4Matrix(ComPtr<ID3D12Device> _dev); // �s��pcbv�𐶐� �߂�l��void�Ȃ̂ɒ���
	void CreateDSVWrapper(ComPtr<ID3D12Device> _dev); //�f�v�X�X�e���V���r���[�쐬
	void CreateCBVSRV4MateriallTextureSph(ComPtr<ID3D12Device> _dev); // �}�e���A���A�e�N�X�`���Asph�t�@�C���p��CBV,SRV���܂Ƃ߂č쐬
	void CreateRTV4Multipass(ComPtr<ID3D12Device> _dev); // �}���`�p�X�pRTV�쐬
	void CreateSRV4Multipass(ComPtr<ID3D12Device> _dev); // �}���`�p�X�pSRV�쐬
	void CreateVertexBufferView();
	void CreateIndexBufferView();


	D3D12_VERTEX_BUFFER_VIEW* GetVbView();
	D3D12_INDEX_BUFFER_VIEW* GetIbView();
};