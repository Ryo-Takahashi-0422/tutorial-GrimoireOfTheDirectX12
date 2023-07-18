#include <stdafx.h>
#include <TextureTransporter.h>

TextureTransporter::TextureTransporter(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator)
{
	pmdMaterialInfo = new PMDMaterialInfo;
	pmdMaterialInfo = _pmdMaterialInfo;

	bufferHeapCreator = _bufferHeapCreator;

	// テクスチャ用転送オブジェクト
	src.resize(pmdMaterialInfo->materialNum);
	dst.resize(pmdMaterialInfo->materialNum);
	texBarriierDesc.resize(pmdMaterialInfo->materialNum);

	// トゥーンテクスチャ用転送オブジェクト
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
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * widthの値は256の倍数であること
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//コピー先設定
		dst[matNum].pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//バリア設定
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
			texBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[matNum]);
			_cmdList->Close();
			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			//コマンドリスト実行完了したかフェンスが通知するまで待機
			
			// 実行中ｺﾏﾝﾄﾞﾘｽﾄ完了後に、指定ﾌｪﾝｽ値をﾌｪﾝｽに書き込むよう設定。ここではｲﾝｸﾘﾒﾝﾄしたfenceValをﾌｪﾝｽに書き込むこととなる。
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) // フェンス現在値が_fenceVal未満ならコマンド実行未完了なので以下待機処理
			{
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event); // フェンスが第一引数に達したらevent発火(シグナル状態にする)
				WaitForSingleObject(event, INFINITE); // eventがシグナル状態になるまで待機する
				CloseHandle(event); // eventハンドルを閉じる(終了させる)
			}
			_cmdAllocator->Reset();//キューをクリア
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
			Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * widthの値は256の倍数であること
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//コピー先設定
		toonDst[matNum].pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//バリア設定
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
			toonBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toonBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			toonBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &toonBarriierDesc[matNum]);
			_cmdList->Close();
			//コマンドリストの実行
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////待ち
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//キューをクリア
			_cmdList->Reset(_cmdAllocator.Get(), nullptr);
		}
	}
}