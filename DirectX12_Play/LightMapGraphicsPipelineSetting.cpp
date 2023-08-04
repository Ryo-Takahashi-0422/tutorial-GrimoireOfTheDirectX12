#include <stdafx.h>
#include <LightMapGraphicsPipelineSetting.h>

LightMapGraphicsPipelineSetting::LightMapGraphicsPipelineSetting(InputLayoutBase* _vertexInputLayout) : /*vertexInputLayout*/IGraphicsPipelineSetting(_vertexInputLayout)
{
	for (int i = 0; i < vertexInputLayout->GetInputLayout().size(); ++i)
	{
		SetInputlayout(i, vertexInputLayout->GetInputLayout()[i]);
	}
}

HRESULT LightMapGraphicsPipelineSetting::CreateGPStateWrapper(ComPtr<ID3D12Device> _dev,
	SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	gpipeLine = SetGPL(setRootSignature, _vsBlob, _psBlob);
	return _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));
}

void LightMapGraphicsPipelineSetting::SetInputlayout(int i, D3D12_INPUT_ELEMENT_DESC inputLayout)
{
	inputLayouts[i] = inputLayout;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC LightMapGraphicsPipelineSetting::SetGPL(
	SetRootSignatureBase* setRootSignature, ComPtr<ID3D10Blob> _vsBlob, ComPtr<ID3D10Blob> _psBlob)
{
	gpipeLine.pRootSignature = setRootSignature->GetRootSignature().Get();

	if (_vsBlob != nullptr)
	{
		gpipeLine.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
		gpipeLine.VS.BytecodeLength = _vsBlob->GetBufferSize();
	}

	if (_psBlob != nullptr)
	{
		gpipeLine.PS.pShaderBytecode = _psBlob->GetBufferPointer();
		gpipeLine.PS.BytecodeLength = _psBlob->GetBufferSize();
	}
	else
	{
		gpipeLine.PS.BytecodeLength = 0;
		gpipeLine.PS.pShaderBytecode = nullptr;
	}
	

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState.MultisampleEnable = false;
	gpipeLine.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeLine.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeLine.RasterizerState.DepthClipEnable = true;

	renderTargetDesc.BlendEnable = false;//�u�����h��L���ɂ��邩�����ɂ��邩
	renderTargetDesc.LogicOpEnable = false;//�_�������L���ɂ��邩�����ɂ��邩
	renderTargetDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeLine.BlendState.AlphaToCoverageEnable = false;
	gpipeLine.BlendState.IndependentBlendEnable = false;
	gpipeLine.BlendState.RenderTarget[0] = renderTargetDesc;
	gpipeLine.InputLayout.pInputElementDescs = inputLayouts;

	gpipeLine.InputLayout.NumElements = _countof(inputLayouts);

	gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 0;

	gpipeLine.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;

	gpipeLine.SampleDesc.Count = 1; //1�T���v��/�s�N�Z��
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = true;
	gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �[�x�o�b�t�@�[�ɐ[�x�l��`������
	gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �\�[�X�f�[�^���R�s�[��f�[�^��菬�����ꍇ��������
	gpipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	return gpipeLine;

}

ComPtr<ID3D12PipelineState> LightMapGraphicsPipelineSetting::GetPipelineState()
{
	return _pipelineState;
}