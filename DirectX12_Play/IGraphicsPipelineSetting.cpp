#include <stdafx.h>
#include <IGraphicsPipelineSetting.h>

IGraphicsPipelineSetting::IGraphicsPipelineSetting(VertexInputLayout* _vertexInputLayout)
{
	vertexInputLayout = _vertexInputLayout;
}

IGraphicsPipelineSetting::~IGraphicsPipelineSetting()
{
	delete this;
}