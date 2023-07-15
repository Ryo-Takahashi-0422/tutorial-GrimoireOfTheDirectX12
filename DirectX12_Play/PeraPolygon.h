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
		{{-1,-1,0.1},{0,1}}, // �E��
		{{-1,1,0.1},{0,1}}, // �E��
	    {{1,-1,0.1},{0,1}}, // �E��
	    {{1,1,0.1},{0,1}} // �E��
	};

	ComPtr<ID3D12Resource> peraBuff = nullptr; // �y���|���S���p�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW peraVBV; // �y���|���S���p���_�r���[

public:
	void CreatePeraView(ComPtr<ID3D12Device> _dev);
};