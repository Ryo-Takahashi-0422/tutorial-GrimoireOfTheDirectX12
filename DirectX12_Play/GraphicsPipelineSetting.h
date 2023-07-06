#pragma once

class GraphicsPipelineSetting
{
private:
	D3D12_INPUT_ELEMENT_DESC inputLayouts[6];
	int count = 0;

public:
	void SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC SetGPL(
		D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine,
		ComPtr<ID3D12Device> _dev,
		ComPtr<ID3D12PipelineState> _pipelineState,
		SetRootSignature* setRootSignature,
		ComPtr<ID3D10Blob> _vsBlob,
		ComPtr<ID3D10Blob> _psBlob);
};