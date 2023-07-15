#pragma once

class PeraLayout
{
private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> peraLayout;

public:
	PeraLayout();
	std::vector<D3D12_INPUT_ELEMENT_DESC> GetPeraLayout();
};