#pragma once

class VertexInputLayout
{
private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
public:
	VertexInputLayout();
	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout();
};