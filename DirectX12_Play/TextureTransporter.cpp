#include <stdafx.h>
#include <TextureTransporter.h>

TextureTransporter::TextureTransporter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator)
{
	pmdMaterialInfo = new PMDMaterialInfo;
	pmdMaterialInfo = _pmdMaterialInfo;

	bufferHeapCreator = _bufferHeapCreator;

	// �e�N�X�`���p�]���I�u�W�F�N�g
	src.resize(pmdMaterialInfo->materialNum);
	dst.resize(pmdMaterialInfo->materialNum);
	texBarriierDesc.resize(pmdMaterialInfo->materialNum);

	// �g�D�[���e�N�X�`���p�]���I�u�W�F�N�g
	toonSrc.resize(pmdMaterialInfo->materialNum);
	toonDst.resize(pmdMaterialInfo->materialNum);
	toonBarriierDesc.resize(pmdMaterialInfo->materialNum);
}

void TextureTransporter::TransportTexture(
	ComPtr<ID3D12GraphicsCommandList> _cmdList,
	ComPtr<ID3D12CommandAllocator> _cmdAllocator,
	ComPtr<ID3D12CommandQueue> _cmdQueue,
	std::vector<DirectX::TexMetadata*> metaData,
	std::vector<DirectX::Image*> img,
	ComPtr<ID3D12Fence> _fence,
	UINT64& _fenceVal)
{
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetTexUploadBuff()[matNum] == nullptr || bufferHeapCreator->GetTexReadBuff()[matNum] == nullptr) continue;

		src[matNum].pResource = bufferHeapCreator->GetTexUploadBuff()[matNum].Get();
		src[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src[matNum].PlacedFootprint.Offset = 0;
		src[matNum].PlacedFootprint.Footprint.Width = metaData[matNum]->width;
		src[matNum].PlacedFootprint.Footprint.Height = metaData[matNum]->height;
		src[matNum].PlacedFootprint.Footprint.Depth = metaData[matNum]->depth;
		src[matNum].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//�R�s�[��ݒ�
		dst[matNum].pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//�o���A�ݒ�
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
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

void TextureTransporter::TransportToonTexture(
	ComPtr<ID3D12GraphicsCommandList> _cmdList,
	ComPtr<ID3D12CommandAllocator> _cmdAllocator,
	ComPtr<ID3D12CommandQueue> _cmdQueue, 
	std::vector<DirectX::TexMetadata*> toonMetaData,
	std::vector<DirectX::Image*> toonImg,
	ComPtr<ID3D12Fence> _fence,
	UINT64& _fenceVal)
{
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetToonUploadBuff()[matNum] == nullptr || bufferHeapCreator->GetToonReadBuff()[matNum] == nullptr) continue;

		toonSrc[matNum].pResource = bufferHeapCreator->GetToonUploadBuff()[matNum].Get();
		toonSrc[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		toonSrc[matNum].PlacedFootprint.Offset = 0;
		toonSrc[matNum].PlacedFootprint.Footprint.Width = toonMetaData[matNum]->width;
		toonSrc[matNum].PlacedFootprint.Footprint.Height = toonMetaData[matNum]->height;
		toonSrc[matNum].PlacedFootprint.Footprint.Depth = toonMetaData[matNum]->depth;
		toonSrc[matNum].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//�R�s�[��ݒ�
		toonDst[matNum].pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//�o���A�ݒ�
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
			toonBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toonBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			toonBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &toonBarriierDesc[matNum]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator.Get(), nullptr);
		}
	}
}