#include <stdafx.h>
#include <PeraLayout.h>

PeraLayout::PeraLayout()
{
	inputLayout =
	{
		//���W
		{
			"POSITION",
			0, // �����Z�}���e�B�N�X�ɑ΂���C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,
			0, // �X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 // ��x�ɕ`�悷��C���X�^���X��
		},

		//uv
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0, // �X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};

}


PeraLayout::~PeraLayout()
{
	delete this;
}