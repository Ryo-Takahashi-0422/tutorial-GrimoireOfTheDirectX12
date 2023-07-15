#pragma once

class IGraphicsPipelineSetting
{
protected:
	VertexInputLayout* vertexInputLayout = nullptr;
	ComPtr<ID3D12PipelineState> _pipelineState = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetDesc = {};
	int count = 0;

//public:
	IGraphicsPipelineSetting(VertexInputLayout* _vertexInputLayout);
	~IGraphicsPipelineSetting();
	virtual HRESULT CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
		SetRootSignature* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob) = 0;

	virtual void SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout) = 0;
	virtual D3D12_GRAPHICS_PIPELINE_STATE_DESC SetGPL(
		SetRootSignature* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob) = 0;

	virtual ComPtr<ID3D12PipelineState> GetPipelineState() = 0;
};