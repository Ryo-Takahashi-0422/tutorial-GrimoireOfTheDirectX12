#pragma once

class ViewCreator
{
private:
	PMDMaterialInfo* pmdMaterialInfo = nullptr;
	BufferHeapCreator* bufferHeapCreator = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vbView = {}; // Vertexビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {}; // (Vertex)Indexビュー
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {}; // 行列用の定数ビュー詳細
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {}; // デプスステンシルビュー詳細
	D3D12_CONSTANT_BUFFER_VIEW_DESC materialTextureSphCBVDesc = {}; // マテリアル情報、テクスチャ、sph用の定数ビュー詳細
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; // シェーダーリソースビュー詳細

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	ComPtr<ID3D12Resource> BlackBuff = nullptr;
	ComPtr<ID3D12Resource> grayTexBuff = nullptr;

	void SetCBVDesc4Matrix();
	void SetDSVDesc();
	void SetCBVDesc4MaterialAndTextureAndSph();
	void SetSRVDesc();

public:
	ViewCreator(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator);
	void CreateCBV4Matrix(ComPtr<ID3D12Device> _dev); // 行列用cbvを生成 戻り値がvoidなのに注意
	void CreateDSVWrapper(ComPtr<ID3D12Device> _dev); //デプスステンシルビュー作成
	void CreateCBV4MateriallTextureSph(ComPtr<ID3D12Device> _dev); // マテリアル、テクスチャ、sphファイル用のcbv,srvをまとめて作成
	void SetVertexBufferView();
	void SetIndexBufferView();

	D3D12_VERTEX_BUFFER_VIEW* GetVbView();
	D3D12_INDEX_BUFFER_VIEW* GetIbView();
};