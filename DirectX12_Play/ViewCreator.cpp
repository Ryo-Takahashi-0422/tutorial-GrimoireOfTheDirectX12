#include <stdafx.h>
#include <ViewCreator.h>

ViewCreator::ViewCreator(PMDMaterialInfo* _pmdMaterialInfo, BufferHeapCreator* _bufferHeapCreator)
	: pmdMaterialInfo(_pmdMaterialInfo), bufferHeapCreator(_bufferHeapCreator)
{}

void ViewCreator::CreateCBV4Matrix(ComPtr<ID3D12Device> _dev)
{
	SetCBVDesc4Matrix();

	_dev->CreateConstantBufferView
	(
		&cbvDesc,
		bufferHeapCreator->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart()
	);

	// �ި������˰�߂��߲���������l�Ȃ�ŏ��̒萔�ޯ̧�ޭ��쐬�Ȃ̂ŁA���ڽ�擾����B���̌�1�ޭ����ڽ�����Z����
	if (basicDescHeapHandle.ptr == 0)
	{
		basicDescHeapHandle = bufferHeapCreator->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart();
	}
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	basicDescHeapHandle.ptr += inc;
}

void ViewCreator::CreateDSVWrapper(ComPtr<ID3D12Device> _dev)
{
	SetDSVDesc();

	_dev->CreateDepthStencilView
	(
		bufferHeapCreator->GetDepthBuff().Get(),
		&dsvDesc,
		bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart()
	);
}

void ViewCreator::CreateCBVSRV4MateriallTextureSph(ComPtr<ID3D12Device> _dev)
{
	//  �ި������˰�߂��߲���������l�Ȃ�ŏ��̒萔�ޯ̧�ޭ��쐬�Ȃ̂ŁA���ڽ�擾����B���̌�1�ޭ����ڽ�����Z����
	if (basicDescHeapHandle.ptr == 0)
	{
		basicDescHeapHandle = bufferHeapCreator->GetCBVSRVHeap()->GetCPUDescriptorHandleForHeapStart();
	}
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SetSRVDesc4MaterialAndTextureAndSph();
	SetCBVDesc4MaterialAndTextureAndSph();

	// �F�e�N�X�`���o�b�t�@�쐬
	bufferHeapCreator->CreateTextureBuffers(_dev);
	// ���e�N�X�`���o�b�t�@
	whiteBuff = bufferHeapCreator->GetWhiteTextureBuff();
	// ���e�N�X�`���o�b�t�@
	BlackBuff = bufferHeapCreator->GetBlackTextureBuff();
	// �O���[�O���f�[�V����
	grayTexBuff = bufferHeapCreator->GetGrayTextureBuff();

	for (int i = 0; i < pmdMaterialInfo->materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialTextureSphCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialTextureSphCBVDesc.BufferLocation += bufferHeapCreator->GetMaterialBuffSize();

		// �e�N�X�`��
		if (bufferHeapCreator->GetTexReadBuff()[i] == nullptr)
		{
			srvDesc4MaterialAndTextureAndSph.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		else
		{
			srvDesc4MaterialAndTextureAndSph.Format = bufferHeapCreator->GetTexReadBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetTexReadBuff()[i].Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// sph�t�@�C��
		if (bufferHeapCreator->GetsphMappedBuff()[i] == nullptr)
		{
			srvDesc4MaterialAndTextureAndSph.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		else
		{
			srvDesc4MaterialAndTextureAndSph.Format = bufferHeapCreator->GetsphMappedBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetsphMappedBuff()[i].Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// spa�t�@�C��
		if (bufferHeapCreator->GetspaMappedBuff()[i] == nullptr)
		{
			srvDesc4MaterialAndTextureAndSph.Format = BlackBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(BlackBuff.Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		else
		{
			srvDesc4MaterialAndTextureAndSph.Format = bufferHeapCreator->GetspaMappedBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetspaMappedBuff()[i].Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// �g�D�[���e�N�X�`���t�@�C��
		if (bufferHeapCreator->GetToonReadBuff()[i] == nullptr)
		{
			srvDesc4MaterialAndTextureAndSph.Format = grayTexBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(grayTexBuff.Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		else
		{
			srvDesc4MaterialAndTextureAndSph.Format = bufferHeapCreator->GetToonReadBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetToonReadBuff()[i].Get(), &srvDesc4MaterialAndTextureAndSph, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;
	}
}

void ViewCreator::CreateRTV4Multipass(ComPtr<ID3D12Device> _dev)
{
	SetRTVDesc4Multipass();

	_dev->CreateRenderTargetView
	(
		bufferHeapCreator->GetMultipassBuff().Get(),
		&multipassRTVDesc,
		bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart()
	);
}

void ViewCreator::CreateSRV4Multipass(ComPtr<ID3D12Device> _dev)
{
	SetSRVDesc4Multipass();
	_dev->CreateShaderResourceView
	(
		bufferHeapCreator->GetMultipassBuff().Get(),
		&multipassSRVDesc,
		bufferHeapCreator->GetMultipassSRVHeap().Get()->GetCPUDescriptorHandleForHeapStart()
	);
}

void ViewCreator::SetVertexBufferView()
{
	vbView.BufferLocation = bufferHeapCreator->GetVertBuff()->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = pmdMaterialInfo->vertices.size();//�S�o�C�g��
	vbView.StrideInBytes = pmdMaterialInfo->pmdvertex_size;//1���_������̃o�C�g��
}

void ViewCreator::SetIndexBufferView()
{
	ibView.BufferLocation = bufferHeapCreator->GetIdxBuff()->GetGPUVirtualAddress();
	ibView.SizeInBytes = sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size();
	ibView.Format = DXGI_FORMAT_R16_UINT;
}

void ViewCreator::SetCBVDesc4Matrix()
{
	cbvDesc.BufferLocation = bufferHeapCreator->GetMatrixBuff()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferHeapCreator->GetMatrixBuff()->GetDesc().Width;
}

void ViewCreator::SetDSVDesc()
{
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
}

void ViewCreator::SetSRVDesc4MaterialAndTextureAndSph()
{
	srvDesc4MaterialAndTextureAndSph.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc4MaterialAndTextureAndSph.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc4MaterialAndTextureAndSph.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc4MaterialAndTextureAndSph.Texture2D.MipLevels = 1;
}

void ViewCreator::SetCBVDesc4MaterialAndTextureAndSph()
{
	materialTextureSphCBVDesc.BufferLocation = bufferHeapCreator->GetMaterialBuff()->GetGPUVirtualAddress();
	materialTextureSphCBVDesc.SizeInBytes = bufferHeapCreator->GetMaterialBuffSize();
}

void ViewCreator::SetRTVDesc4Multipass()
{
	multipassRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	multipassRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
}

void ViewCreator::SetSRVDesc4Multipass()
{
	multipassSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	multipassSRVDesc.Format = multipassRTVDesc.Format;
	multipassSRVDesc.Texture2D.MipLevels = 1;
	multipassSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
}

D3D12_VERTEX_BUFFER_VIEW* ViewCreator::GetVbView()
{
	return &vbView;
}

D3D12_INDEX_BUFFER_VIEW* ViewCreator::GetIbView()
{
	return &ibView;
}