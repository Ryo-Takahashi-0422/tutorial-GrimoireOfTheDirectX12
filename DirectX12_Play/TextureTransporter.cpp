#include <stdafx.h>
#include <TextureTransporter.h>

TextureTransporter::TextureTransporter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator)
{
	pmdMaterialInfo = new PMDMaterialInfo;
	pmdMaterialInfo = _pmdMaterialInfo;

	bufferHeapCreator = _bufferHeapCreator;
}

void TextureTransporter::TransportPMDMaterialTexture(
	ComPtr<ID3D12GraphicsCommandList> _cmdList,
	ComPtr<ID3D12CommandAllocator> _cmdAllocator,
	ComPtr<ID3D12CommandQueue> _cmdQueue,
	std::vector<DirectX::TexMetadata*> metaData,
	std::vector<DirectX::Image*> img,
	ComPtr<ID3D12Fence> _fence,
	UINT64& _fenceVal,
	std::vector<ComPtr<ID3D12Resource>> uploadBuff,
	std::vector<ComPtr<ID3D12Resource>> readBuff,
	unsigned int itCount)
{
	// �e�N�X�`���p�]���I�u�W�F�N�g�̃��T�C�Y
	pmdSource.resize(pmdMaterialInfo->materialNum);
	pmdDestination.resize(pmdMaterialInfo->materialNum);
	texBarriierDesc.resize(pmdMaterialInfo->materialNum);

	for (int count = 0; count < itCount; count++)
	{
		if (uploadBuff[count] == nullptr || readBuff[count] == nullptr) continue;

		pmdSource[count].pResource = uploadBuff[count].Get();
		pmdSource[count].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		pmdSource[count].PlacedFootprint.Offset = 0;
		pmdSource[count].PlacedFootprint.Footprint.Width = metaData[count]->width;
		pmdSource[count].PlacedFootprint.Footprint.Height = metaData[count]->height;
		pmdSource[count].PlacedFootprint.Footprint.Depth = metaData[count]->depth;
		pmdSource[count].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(img[count]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		pmdSource[count].PlacedFootprint.Footprint.Format = img[count]->format;//metaData.format;

		//�R�s�[��ݒ�
		pmdDestination[count].pResource = readBuff[count].Get();
		pmdDestination[count].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		pmdDestination[count].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&pmdDestination[count], 0, 0, 0, &pmdSource[count], nullptr);

			//�o���A�ݒ�...�����Ƃ��AStateAfter��...Generic_Read�Ȃǂɂ��Ă����s�\�B�����L�ڌ������炸�ڍוs���B
			texBarriierDesc[count].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[count].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[count].Transition.pResource = readBuff[count].Get();
			texBarriierDesc[count].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[count].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[count].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[count]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			//�R�}���h���X�g���s�����������t�F���X���ʒm����܂őҋ@
			
			// ���s�������ؽĊ�����ɁA�w��̪ݽ�l��̪ݽ�ɏ������ނ悤�ݒ�B�����łͲݸ���Ă���fenceVal��̪ݽ�ɏ������ނ��ƂƂȂ�B
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) // �t�F���X���ݒl��_fenceVal�����Ȃ�R�}���h���s�������Ȃ̂ňȉ��ҋ@����
			{
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event); // �t�F���X���������ɒB������event����(�V�O�i����Ԃɂ���)
				WaitForSingleObject(event, INFINITE); // event���V�O�i����ԂɂȂ�܂őҋ@����
				CloseHandle(event); // event�n���h�������(�I��������)
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator.Get(), nullptr);
		}
	}
}