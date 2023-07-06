#include <stdafx.h>
#include <BufferHeapCreator.h>

BufferHeapCreator::BufferHeapCreator(PMDMaterialInfo* _pmdMaterialInfo,  PrepareRenderingWindow* _prepareRenderingWindow, TextureLoader* _textureLoader)
{
	pmdMaterialInfo = new PMDMaterialInfo;
	pmdMaterialInfo = _pmdMaterialInfo;

	prepareRenderingWindow = new PrepareRenderingWindow;
	prepareRenderingWindow = _prepareRenderingWindow;

	textureLoader = new TextureLoader;
	textureLoader = _textureLoader;

	texUploadBuff.resize(pmdMaterialInfo->materialNum);//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	texReadBuff.resize(pmdMaterialInfo->materialNum);//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	sphMappedBuff.resize(pmdMaterialInfo->materialNum);//sph�p�o�b�t�@�[
	spaMappedBuff.resize(pmdMaterialInfo->materialNum);//spa�p�o�b�t�@�[
	toonUploadBuff.resize(pmdMaterialInfo->materialNum);//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	toonReadBuff.resize(pmdMaterialInfo->materialNum);//�g�D�[���p���[�h�o�b�t�@�[
}

void BufferHeapCreator::SetVertexAndIndexHeapProp(D3D12_HEAP_PROPERTIES* heapProps)
{
	heapProps->Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps->MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps->CreationNodeMask = 0;
	heapProps->VisibleNodeMask = 0;
}

void BufferHeapCreator::SetDepthHeapProp(D3D12_HEAP_PROPERTIES* depthHeapProps)
{
	depthHeapProps->Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProps->CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProps->MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void BufferHeapCreator::SetDepthResourceDesc(D3D12_RESOURCE_DESC* depthResDesc)
{
	depthResDesc->Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc->Width = prepareRenderingWindow->GetWindowWidth();
	depthResDesc->Height = prepareRenderingWindow->GetWindowHeight();
	depthResDesc->DepthOrArraySize = 1;
	depthResDesc->Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l�������ݗp
	depthResDesc->SampleDesc.Count = 1; // 1pixce/1�̃T���v��
	depthResDesc->Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
}

void BufferHeapCreator::SetClearValue(D3D12_CLEAR_VALUE* depthClearValue)
{
	depthClearValue->Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue->DepthStencil.Depth = 1.0f; // �[��1.0(�ő�l)�ŃN���A
}

HRESULT BufferHeapCreator::CreateBufferOfVertex(ComPtr<ID3D12Device> _dev, 
	D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& vertresDesc)
{
	return _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // ���\�[�X�̏�ԁBGPU���炵�ēǂݎ��p
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfIndex(ComPtr<ID3D12Device> _dev,
	D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC& indicesDesc)
{
	return _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfDepth(ComPtr<ID3D12Device> _dev,
	D3D12_HEAP_PROPERTIES& depthHeapProps, D3D12_RESOURCE_DESC& depthResDesc)
{
	return _dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(depthBuff.ReleaseAndGetAddressOf())
	);
}

void BufferHeapCreator::CreateUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
	std::string strModelPath, std::vector<DirectX::TexMetadata*>& metaData, std::vector<DirectX::Image*>& img)
{
	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	for (int i = 0; i < pmdMaterialInfo->materials.size(); i++)
	{
		if (strlen(pmdMaterialInfo->materials[i].addtional.texPath.c_str()) == 0)
		{
			texUploadBuff[i] = nullptr;
			texReadBuff[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterialInfo->materials[i].addtional.texPath;

		// �t�@�C������*���܂ޏꍇ�̏���
		if (std::count(std::begin(texFileName), std::end(texFileName), '*') > 0)
		{
			auto namePair = Utility::SplitFileName(texFileName);

			if (Utility::GetExtension(namePair.first) == "sph" || Utility::GetExtension(namePair.first) == "spa")
			{
				texFileName = namePair.second;
			}

			else
			{
				texFileName = namePair.first;
			}
		}

		// spa,sph�g���q�t�@�C����slicepitch���傫�����ăI�[�o�[�t���[?���邽�߁A�o�b�t�@�[�쐬�Ɏ��s����B
		// �X�ɏڍׂ͕s����������ɂ��Ȃ���bufferHeapCreator->GetVertBuff()�̃}�b�s���O�����s����悤�ɂȂ邽�߁A�ꎞ�������

		auto texFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, texFileName.c_str());
		auto wTexPath = Utility::GetWideStringFromSring(texFilePath);
		auto extention = Utility::GetExtension(texFilePath);

		if (!textureLoader->GetTable().count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			return;
		}
		metaData[i] = new TexMetadata;
		result = textureLoader->GetTable()[extention](wTexPath, metaData[i], scratchImg);

		if (scratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector �̌^��const�K�p����ƃR���p�C���ɂ�苓�����ω����邽�ߋ֎~
		img[i] = new Image;
		img[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		img[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		img[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		img[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		img[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		img[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// CPU�哱��GPU��sph�t�@�C���̃o�b�t�@�����E�T�u���\�[�X�փR�s�[
		// �v���t�@�N�^�����O

		if (Utility::GetExtension(texFileName) == "sph")
		{
			sphMappedBuff[i] = CreateD3DX12ResourceBuffer::CreateMappedSphSpaTexResource(_dev, metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			spaMappedBuff[i] = nullptr;
		}

		else if (Utility::GetExtension(texFileName) == "spa")
		{
			spaMappedBuff[i] = CreateD3DX12ResourceBuffer::CreateMappedSphSpaTexResource(_dev, metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			sphMappedBuff[i] = nullptr;
		}

		else
		{
			std::tie(texUploadBuff[i], texReadBuff[i]) = CreateD3DX12ResourceBuffer::LoadTextureFromFile(_dev, metaData[i], img[i], texFilePath);
			sphMappedBuff[i] = nullptr;
			spaMappedBuff[i] = nullptr;
		}
	}
}

void BufferHeapCreator::CreateToonUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
	std::string strModelPath,
	std::vector<DirectX::TexMetadata*>& toonMetaData,
	std::vector<DirectX::Image*>& toonImg)
{

	for (int i = 0; i < pmdMaterialInfo->materials.size(); i++)
	{
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFileName[16];
		sprintf(toonFileName, "toon%02d.bmp", pmdMaterialInfo->materials[i].addtional.toonIdx + 1);
		toonFilePath += toonFileName;
		toonFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, toonFilePath.c_str());

		auto wTexPath = Utility::GetWideStringFromSring(toonFilePath);
		auto extention = Utility::GetExtension(toonFilePath);

		if (!textureLoader->GetTable().count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			//return 0;
			break;
		}

		toonMetaData[i] = new TexMetadata;
		result = textureLoader->GetTable()[extention](wTexPath, toonMetaData[i], toonScratchImg);

		if (toonScratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector �̌^��const�K�p����ƃR���p�C���ɂ�苓�����ω����邽�ߋ֎~
		toonImg[i] = new Image;
		toonImg[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		toonImg[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		toonImg[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		toonImg[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		toonImg[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		toonImg[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// tooIdx�w��(+1)��toon�t�@�C�������݂���ꍇ
		if (_stat(toonFilePath.c_str(), &s) == 0)
		{
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = CreateD3DX12ResourceBuffer::LoadTextureFromFile(_dev, toonMetaData[i], toonImg[i], toonFilePath);
		}

		else
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
	}
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetVertBuff()
{
	return vertBuff;
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetIdxBuff()
{
	return idxBuff;
}

ComPtr<ID3D12Resource> BufferHeapCreator::GetDepthBuff()
{
	return depthBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetTexUploadBuff()
{
	return texUploadBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetTexReadBuff()
{
	return texReadBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetsphMappedBuff()
{
	return sphMappedBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetspaMappedBuff()
{
	return spaMappedBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetToonUploadBuff()
{
	return toonUploadBuff;
}

std::vector<ComPtr<ID3D12Resource>> BufferHeapCreator::GetToonReadBuff()
{
	return toonReadBuff;
}