#pragma once

struct PeraVertex
{
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

class PeraPolygon
{
private:
	PeraVertex pv[4] =
	{
		{{-1,-1,0.1},{0,1}}, // 右上
		{{-1,1,0.1},{0,1}}, // 右上
	    {{1,-1,0.1},{0,1}}, // 右上
	    {{1,1,0.1},{0,1}} // 右上
	};

	ComPtr<ID3D12Resource> peraBuff = nullptr; // ペラポリゴン用バッファ
	D3D12_VERTEX_BUFFER_VIEW peraVBV; // ペラポリゴン用頂点ビュー

public:
	void CreatePeraView(ComPtr<ID3D12Device> _dev);
};