#pragma once

class PeraGraphicsPipelineSetting : public IGraphicsPipelineSetting
{

private:
	D3D12_INPUT_ELEMENT_DESC inputLayouts[2];

public:
	PeraGraphicsPipelineSetting(VertexInputLayout* _vertexInputLayout);
	~PeraGraphicsPipelineSetting();
	HRESULT CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
		SetRootSignature* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob);

	void SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC SetGPL(
		SetRootSignature* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob);

	ComPtr<ID3D12PipelineState> GetPipelineState();
};