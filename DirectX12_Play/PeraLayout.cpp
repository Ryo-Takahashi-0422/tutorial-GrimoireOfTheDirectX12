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

		// ���̃V�F�[�_�[�ݒ��4�_�̈ꖇ�|����DirectX12�ɐ����������f�[�^�𗘗p����B
		// ���̂��߁APMD���f���o�͐ݒ�̂悤��NORMAL(�Ȃǂ̾��è��)��ǉ�����ƁA�z���ł͂��邪�f�[�^�����݂��Ȃ����߂ɑΉ�����V�F�[�_�[����
		// �n��f�[�^�|�C���^������Ă��܂��A���̏ꍇ(������NORMAL�w�肷��)TEXCOORD(uv)�f�[�^���n�炸�T���v�����O�ł��Ȃ��Ȃ�\�����o�O��B

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