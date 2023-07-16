#pragma once

class GraphicsPipelineSetting : public IGraphicsPipelineSetting
{
private:
	//VertexInputLayout* vertexInputLayout = nullptr;
	//ComPtr<ID3D12PipelineState> _pipelineState = nullptr;
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	//D3D12_RENDER_TARGET_BLEND_DESC renderTargetDesc = {};
	D3D12_INPUT_ELEMENT_DESC inputLayouts[6];
	//int count = 0;

public:
	GraphicsPipelineSetting(InputLayoutBase* _vertexInputLayout);
	HRESULT CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
		SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob);

	void SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC SetGPL(
		SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob);

	ComPtr<ID3D12PipelineState> GetPipelineState();
};