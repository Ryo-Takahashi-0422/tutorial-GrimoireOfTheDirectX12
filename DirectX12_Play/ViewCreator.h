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
	D3D12_CONSTANT_BUFFER_VIEW_DESC materialTextureSphCBVDesc = {}; // �}�e���A�����A�e�N�X�`���Asph�p�̒萔�r���[�ڍ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; // �V�F�[�_�[���\�[�X�r���[�ڍ�

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	void SetCBVDesc4Matrix();
	void SetDSVDesc();
	void SetCBVDesc4MaterialAndTextureAndSph();
	void SetSRVDesc();

public:
	ViewCreator(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);
	void CreateCBV4Matrix(ComPtr<ID3D12Device> _dev); // �s��pcbv�𐶐� �߂�l��void�Ȃ̂ɒ���
	void CreateDSVWrapper(ComPtr<ID3D12Device> _dev); //�f�v�X�X�e���V���r���[�쐬
	void CreateCBV4MateriallTextureSph(ComPtr<ID3D12Device> _dev); // �}�e���A���A�e�N�X�`���Asph�t�@�C���p��cbv,srv���܂Ƃ߂č쐬
	void SetVertexBufferView();
	void SetIndexBufferView();

	D3D12_VERTEX_BUFFER_VIEW* GetVbView();
	D3D12_INDEX_BUFFER_VIEW* GetIbView();
};