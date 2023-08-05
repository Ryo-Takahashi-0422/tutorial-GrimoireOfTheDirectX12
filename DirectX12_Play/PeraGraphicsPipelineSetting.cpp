#include <stdafx.h>
#include <PeraGraphicsPipelineSetting.h>

PeraGraphicsPipelineSetting::PeraGraphicsPipelineSetting(InputLayoutBase* _vertexInputLayout) : IGraphicsPipelineSetting(_vertexInputLayout)
{
	for (int i = 0; i < vertexInputLayout->GetInputLayout().size(); ++i)
	{
		SetInputlayout(i, vertexInputLayout->GetInputLayout()[i]);
	}
}

PeraGraphicsPipelineSetting::~PeraGraphicsPipelineSetting()
{
}

HRESULT PeraGraphicsPipelineSetting::CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
	SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	gpipeLine = SetGPL(setRootSignature, _vsBlob, _psBlob);
	// 一個目
	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// 二個目
	return _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState2.ReleaseAndGetAddressOf()));
}

void PeraGraphicsPipelineSetting::SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout)
{
	inputLayouts[i] = inputLayout;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PeraGraphicsPipelineSetting::SetGPL(
	SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	gpipeLine.pRootSignature = setRootSignature->GetRootSignature().Get();

	gpipeLine.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());

	gpipeLine.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//renderTargetDesc.BlendEnable = false;//ブレンドを有効にするか無効にするか
	//renderTargetDesc.LogicOpEnable = false;//論理操作を有効にするか無効にするか
	//renderTargetDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeLine.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeLine.InputLayout.NumElements = _countof(inputLayouts);
	gpipeLine.InputLayout.pInputElementDescs = inputLayouts;

	//gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 1;
	gpipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpipeLine.SampleDesc.Count = 1; //1サンプル/ピクセル
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = false;
	gpipeLine.DepthStencilState.StencilEnable = false;
	//gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度バッファーに深度値を描き込む
	//gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // ソースデータがコピー先データより小さい場合書き込む
	//gpipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpipeLine.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	return gpipeLine;

}

ComPtr<ID3D12PipelineState> PeraGraphicsPipelineSetting::GetPipelineState()
{
	return _pipelineState;
}

ComPtr<ID3D12PipelineState> PeraGraphicsPipelineSetting::GetPipelineState2()
{
	return _pipelineState2;
}