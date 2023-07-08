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

void MappingExecuter::TransferTexUploadToBuff(std::vector<DirectX::Image*> img)
{
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetTexUploadBuff()[matNum] == nullptr) continue;

		auto srcAddress = img[matNum]->pixels;
		auto rowPitch = Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		result = bufferHeapCreator->GetTexUploadBuff()[matNum]->Map(0, nullptr, (void**)&mapforImg);

		// img:���f�[�^�̏����A�h���X(srcAddress)�����s�b�`���I�t�Z�b�g���Ȃ���A�␳�����s�b�`��(rowPitch)�̃A�h���X��
		// mapforImg�ɂ��̐���(rowPitch)�I�t�Z�b�g���J��Ԃ��R�s�[���Ă���
		for (int i = 0; i < img[matNum]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[matNum]->rowPitch;
			mapforImg += rowPitch;
		}

		bufferHeapCreator->GetTexUploadBuff()[matNum]->Unmap(0, nullptr);
	}
}

void MappingExecuter::TransferToonTexUploadToBuff(std::vector<DirectX::Image*> toonImg)
{
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetToonUploadBuff()[matNum] == nullptr) continue;

		auto toonSrcAddress = toonImg[matNum]->pixels;
		auto toonrowPitch = Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		result = bufferHeapCreator->GetToonUploadBuff()[matNum]->Map(0, nullptr, (void**)&toonmapforImg);

		for (int i = 0; i < toonImg[matNum]->height; ++i)
		{
			std::copy_n(toonSrcAddress, toonrowPitch, toonmapforImg);
			toonSrcAddress += toonImg[matNum]->rowPitch;
			toonmapforImg += toonrowPitch;
		}

		bufferHeapCreator->GetToonUploadBuff()[matNum]->Unmap(0, nullptr);
	}
}