#include <stdafx.h>
#include <GraphicsPipelineSetting.h>

void GraphicsPipelineSetting::SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout)
{
	inputLayouts[i] = inputLayout;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineSetting::SetGPL(
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine,
	ComPtr<ID3D12Device> _dev,
	ComPtr<ID3D12PipelineState> _pipelineState,
	SetRootSignature* setRootSignature,
	ComPtr<ID3D10Blob> _vsBlob,
	ComPtr<ID3D10Blob> _psBlob)
{
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	gpipeLine.pRootSignature = setRootSignature->GetRootSignature().Get();
	//gpipeLine.pRootSignature = setRootSignature->GetRootSignature();

	gpipeLine.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeLine.VS.BytecodeLength = _vsBlob->GetBufferSize();

	gpipeLine.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeLine.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState.MultisampleEnable = false;
	gpipeLine.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeLine.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeLine.RasterizerState.DepthClipEnable = true;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetdDesc = {};
	renderTargetdDesc.BlendEnable = false;//ブレンドを有効にするか無効にするか
	renderTargetdDesc.LogicOpEnable = false;//論理操作を有効にするか無効にするか
	renderTargetdDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeLine.BlendState.AlphaToCoverageEnable = false;
	gpipeLine.BlendState.IndependentBlendEnable = false;
	gpipeLine.BlendState.RenderTarget[0] = renderTargetdDesc;
	gpipeLine.InputLayout.pInputElementDescs = inputLayouts;

	gpipeLine.InputLayout.NumElements = _countof(inputLayouts);

	gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 1;

	gpipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	gpipeLine.SampleDesc.Count = 1; //1サンプル/ピクセル
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = true;
	gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度バッファーに深度値を描き込む
	gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // ソースデータがコピー先データより小さい場合書き込む
	gpipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	return gpipeLine;

}