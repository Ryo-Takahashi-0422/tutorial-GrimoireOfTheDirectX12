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
		bufferHeapCreator->GetMatrixHeap()->GetCPUDescriptorHandleForHeapStart()//basicDescHeapHandle
	);
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

void ViewCreator::CreateCBV4MateriallTextureSph(ComPtr<ID3D12Device> _dev)
{
	auto basicDescHeapHandle = bufferHeapCreator->GetMatrixHeap()->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	basicDescHeapHandle.ptr += inc;

	SetSRVDesc();
	SetCBVDesc4MaterialAndTextureAndSph();

	// ���e�N�X�`���o�b�t�@
	whiteBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0xff);

	// ���e�N�X�`���o�b�t�@
	BlackBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0x00);

	// �O���[�O���f�[�V����
	grayTexBuff = CreateD3DX12ResourceBuffer::CreateGrayGradationTexture(_dev);

	for (int i = 0; i < pmdMaterialInfo->materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialTextureSphCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialTextureSphCBVDesc.BufferLocation += bufferHeapCreator->GetMaterialBuffSize();

		// �e�N�X�`��
		if (bufferHeapCreator->GetTexReadBuff()[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = bufferHeapCreator->GetTexReadBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetTexReadBuff()[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// sph�t�@�C��
		if (bufferHeapCreator->GetsphMappedBuff()[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = bufferHeapCreator->GetsphMappedBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetsphMappedBuff()[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// spa�t�@�C��
		if (bufferHeapCreator->GetspaMappedBuff()[i] == nullptr)
		{
			srvDesc.Format = BlackBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(BlackBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = bufferHeapCreator->GetspaMappedBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetspaMappedBuff()[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// �g�D�[���e�N�X�`���t�@�C��
		if (bufferHeapCreator->GetToonReadBuff()[i] == nullptr)
		{
			srvDesc.Format = grayTexBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(grayTexBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = bufferHeapCreator->GetToonReadBuff()[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(bufferHeapCreator->GetToonReadBuff()[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;
	}
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

void ViewCreator::SetSRVDesc()
{
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
}

void ViewCreator::SetCBVDesc4MaterialAndTextureAndSph()
{
	materialTextureSphCBVDesc.BufferLocation = bufferHeapCreator->GetMaterialBuff()->GetGPUVirtualAddress();
	materialTextureSphCBVDesc.SizeInBytes = bufferHeapCreator->GetMaterialBuffSize();
}

D3D12_VERTEX_BUFFER_VIEW* ViewCreator::GetVbView()
{
	return &vbView;
}

D3D12_INDEX_BUFFER_VIEW* ViewCreator::GetIbView()
{
	return &ibView;
}