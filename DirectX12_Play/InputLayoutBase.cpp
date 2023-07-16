#include <stdafx.h>
#include <InputLayoutBase.h>

InputLayoutBase::~InputLayoutBase()
{
	delete this;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayoutBase::GetInputLayout()
{
	return inputLayout;
}

size_t InputLayoutBase::GetInputSize()
{
	return inputLayout.size();
}