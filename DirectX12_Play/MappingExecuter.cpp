#include <stdafx.h>
#include <MappingExecuter.h>

MappingExecuter::MappingExecuter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator)
{
	pmdMaterialInfo = _pmdMaterialInfo;
	bufferHeapCreator = _bufferHeapCreator;
}

void MappingExecuter::MappingVertBuff()
{
	//CPU�͈ÖٓI�ȃq�[�v�̏��𓾂��Ȃ����߁AMap�֐��ɂ��VRAM��̃o�b�t�@�[�ɃA�h���X�����蓖�Ă���Ԃ�
	//���_�Ȃǂ̏���VRAM�փR�s�[���Ă���(���̂R��CPU��GPU�ǂ�����A�N�Z�X�\��UPLOAD�^�C�v�ȃq�[�v�̃}�b�v�\)�A
	//�Ƃ��������BUnmap�̓R�����g�A�E�g���Ă����ɉe���͂Ȃ���...
	result = bufferHeapCreator->GetVertBuff()->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(pmdMaterialInfo->vertices), std::end(pmdMaterialInfo->vertices), vertMap);
	bufferHeapCreator->GetVertBuff()->Unmap(0, nullptr);
}

void MappingExecuter::MappingIndexOfVertexBuff()
{
	result = bufferHeapCreator->GetIdxBuff()->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(pmdMaterialInfo->indices), std::end(pmdMaterialInfo->indices), mappedIdx);
	bufferHeapCreator->GetIdxBuff()->Unmap(0, nullptr);
}

void MappingExecuter::MappingMaterialBuff()
{
	result = bufferHeapCreator->GetMaterialBuff()->Map(0, nullptr, (void**)&mapMaterial);
	for (auto m : pmdMaterialInfo->materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += bufferHeapCreator->GetMaterialBuffSize();
	}
	bufferHeapCreator->GetMaterialBuff()->Unmap(0, nullptr);
}

void MappingExecuter::TransferTexUploadToBuff(std::vector<ComPtr<ID3D12Resource>> uploadBuff, std::vector<DirectX::Image*> img, unsigned int itCount)
{
	for (int count = 0; count < itCount; count++)
	{
		if (uploadBuff[count] == nullptr) continue;

		auto srcAddress = img[count]->pixels;
		auto rowPitch = Utility::AlignmentSize(img[count]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		result = uploadBuff[count]->Map(0, nullptr, (void**)&mapforImg);

		// img:���f�[�^�̏����A�h���X(srcAddress)�����s�b�`���I�t�Z�b�g���Ȃ���A�␳�����s�b�`��(rowPitch)�̃A�h���X��
		// mapforImg�ɂ��̐���(rowPitch)�I�t�Z�b�g���J��Ԃ��R�s�[���Ă���
		for (int i = 0; i < img[count]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[count]->rowPitch;
			mapforImg += rowPitch;
		}

		uploadBuff[count]->Unmap(0, nullptr);
	}
}

void MappingExecuter::MappingGaussianWeight(std::vector<float> weights)
{
	bufferHeapCreator->GetGaussianBuff()->Map(0, nullptr, (void**)&mappedweight);
	std::copy(weights.begin(), weights.end(), mappedweight);
	bufferHeapCreator->GetGaussianBuff()->Unmap(0, nullptr);
}