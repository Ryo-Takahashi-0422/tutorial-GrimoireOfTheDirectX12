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
		{{-1.0f,-1.0f,0.1f},{0.0f,1.0f}}, // ����
		{{-1.0f,1.0f,0.1f},{0.0f,0.0f}}, // ����
	    {{1.0f,-1.0f,0.1f},{1.0f,1.0f}}, // �E��
	    {{1.0f,1.0f,0.1f},{1.0f,0.0f}} // �E��
	};

	PeraVertex* mappedPera = nullptr;
	ComPtr<ID3D12Resource> peraBuff = nullptr; // �y���|���S���p�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW peraVBV = {}; // �y���|���S���p���_�r���[

public:
	void CreatePeraView(ComPtr<ID3D12Device> _dev);
	D3D12_VERTEX_BUFFER_VIEW* GetVBView();
};