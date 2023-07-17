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
	delete this;
}

HRESULT PeraGraphicsPipelineSetting::CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
	SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	gpipeLine = SetGPL(setRootSignature, _vsBlob, _psBlob);
	// ���
	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// ���
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

	//gpipeLine.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	//gpipeLine.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeLine.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());

	//gpipeLine.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	//gpipeLine.PS.BytecodeLength = _psBlob->GetBufferSize();
	gpipeLine.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//renderTargetDesc.BlendEnable = false;//�u�����h��L���ɂ��邩�����ɂ��邩
	//renderTargetDesc.LogicOpEnable = false;//�_�������L���ɂ��邩�����ɂ��邩
	//renderTargetDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeLine.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeLine.InputLayout.NumElements = _countof(inputLayouts);
	gpipeLine.InputLayout.pInputElementDescs = inputLayouts;

	//gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 1;

	gpipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpipeLine.SampleDesc.Count = 1; //1�T���v��/�s�N�Z��
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = false;
	gpipeLine.DepthStencilState.StencilEnable = false;
	//gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �[�x�o�b�t�@�[�ɐ[�x�l��`������
	//gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �\�[�X�f�[�^���R�s�[��f�[�^��菬�����ꍇ��������
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