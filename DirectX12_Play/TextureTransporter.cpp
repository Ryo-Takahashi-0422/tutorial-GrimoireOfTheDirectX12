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
	// テクスチャ用転送オブジェクトのリサイズ
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
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * widthの値は256の倍数であること
		pmdSource[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//コピー先設定
		pmdDestination[matNum].pResource = /*bufferHeapCreator->GetPMDTexReadBuff()*/readBuff[matNum].Get();
		pmdDestination[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		pmdDestination[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&pmdDestination[matNum], 0, 0, 0, &pmdSource[matNum], nullptr);

			//バリア設定
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = /*bufferHeapCreator->GetPMDTexReadBuff()*/readBuff[matNum].Get();
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