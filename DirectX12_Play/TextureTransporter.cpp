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
	std::vector<ComPtr<ID3D12Resource>> readBuff)
{
	// �e�N�X�`���p�]���I�u�W�F�N�g�̃��T�C�Y
	pmdSource.resize(pmdMaterialInfo->materialNum);
	pmdDestination.resize(pmdMaterialInfo->materialNum);
	texBarriierDesc.resize(pmdMaterialInfo->materialNum);

	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (/*bufferHeapCreator->GetPMDTexUploadBuff()*/uploadBuff[matNum] == nullptr || /*bufferHeapCreator->GetPMDTexReadBuff()*/readBuff[matNum] == nullptr) continue;

		pmdSource[matNum].pResource = /*bufferHeapCreator->GetPMDTexUploadBuff()*/uploadBuff[matNum].Get();
		pmdSource[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		pmdSource[matNum].PlacedFootprint.Offset = 0;
		pmdSource[matNum].PlacedFootprint.Footprint.Width = metaData[matNum]->width;
		pmdSource[matNum].PlacedFootprint.Footprint.Height = metaData[matNum]->height;
		pmdSource[matNum].PlacedFootprint.Footprint.Depth = metaData[matNum]->depth;
		pmdSource[matNum].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		pmdSource[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//�R�s�[��ݒ�
		pmdDestination[matNum].pResource = /*bufferHeapCreator->GetPMDTexReadBuff()*/readBuff[matNum].Get();
		pmdDestination[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		pmdDestination[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&pmdDestination[matNum], 0, 0, 0, &pmdSource[matNum], nullptr);

			//�o���A�ݒ�
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = /*bufferHeapCreator->GetPMDTexReadBuff()*/readBuff[matNum].Get();
			texBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[matNum]);
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