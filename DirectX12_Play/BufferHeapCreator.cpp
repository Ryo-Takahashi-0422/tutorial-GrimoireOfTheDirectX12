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

	pmdTexUploadBuff.resize(pmdMaterialInfo->materialNum);//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	pmdTexReadBuff.resize(pmdMaterialInfo->materialNum);//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	sphMappedBuff.resize(pmdMaterialInfo->materialNum);//sph�p�o�b�t�@�[
	spaMappedBuff.resize(pmdMaterialInfo->materialNum);//spa�p�o�b�t�@�[
	toonUploadBuff.resize(pmdMaterialInfo->materialNum);//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	toonReadBuff.resize(pmdMaterialInfo->materialNum);//�g�D�[���p���[�h�o�b�t�@�[

	materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;
}

void BufferHeapCreator::SetRTVHeapDesc()
{
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
}

void BufferHeapCreator::SetDSVHeapDesc()
{
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 3; // �[�x + lightmap + AO�p
}

void BufferHeapCreator::SetCBVSRVHeapDesc()
{
	cbvsrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvsrvHeapDesc.NumDescriptors = 1 + pmdMaterialInfo->materialNum * 5 + 2; // �s��cbv + (material cbv+�e�N�X�`��srv+sph srv+spa srv+toon srv)*materialNum + �[�x�}�b�v + ���C�g�}�b�v
	cbvsrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvsrvHeapDesc.NodeMask = 0;
}

void BufferHeapCreator::SetMutipassRTVHeapDesc()
{
	mutipassRTVHeapDesc = rtvHeaps->GetDesc(); // �����̃q�[�v����ݒ�p��
	mutipassRTVHeapDesc.NumDescriptors = 8; // �}���`�p�X2�� + �}���`�^�[�Q�b�g1�� + bloom*2 + shrinkedModel + AO + imgui
}

void BufferHeapCreator::SetMutipassSRVHeapDesc()
{
	mutipassSRVHeapDesc = rtvHeaps->GetDesc(); // �����̃q�[�v����ݒ�p��
	mutipassSRVHeapDesc.NumDescriptors = 14; // �}���`�p�X�Ώې��ŕϓ����� + effectCBV + normalmapSRV + shadow + lightmap + �V�[���s�� + �}���`�^�[�Q�b�g + bloom*2 + shrinkedModel + AO + imgui + imgui PostSetting
	mutipassSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	mutipassSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
}

void BufferHeapCreator::SetVertexHeapProp()
{
	vertexHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	vertexHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	vertexHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	vertexHeapProps.CreationNodeMask = 0;
	vertexHeapProps.VisibleNodeMask = 0;
}

void BufferHeapCreator::SetDepthHeapProp()
{
	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void BufferHeapCreator::SetDepthResourceDesc()
{
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = prepareRenderingWindow->GetWindowWidth();
	depthResDesc.Height = prepareRenderingWindow->GetWindowHeight();
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS; // �[�x�l�������ݗp
	depthResDesc.SampleDesc.Count = 1; // 1pixce/1�̃T���v��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
}

void BufferHeapCreator::SetClearValue()
{
	float clsClr[4] = { 0.5,0.5,0.5,1.0 };
	depthClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);
	//depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	//depthClearValue.DepthStencil.Depth = 1.0f; // �[��1.0(�ő�l)�ŃN���A

}

HRESULT BufferHeapCreator::CreateRTVHeap(ComPtr<ID3D12Device> _dev)
{
	return _dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));
	
}

HRESULT BufferHeapCreator::CreateDSVHeap(ComPtr<ID3D12Device> _dev)
{
	SetDSVHeapDesc();
	return _dev->CreateDescriptorHeap
	(
		&dsvHeapDesc,
		IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateCBVSRVHeap(ComPtr<ID3D12Device> _dev)
{
	SetCBVSRVHeapDesc();
	return _dev->CreateDescriptorHeap
	(
		&cbvsrvHeapDesc,
		IID_PPV_ARGS(cbvsrvHeap.GetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateMultipassRTVHeap(ComPtr<ID3D12Device> _dev)
{
	SetMutipassRTVHeapDesc();

	return _dev->CreateDescriptorHeap
	(
		&mutipassRTVHeapDesc,
		IID_PPV_ARGS(multipassRTVHeap.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateMultipassSRVHeap(ComPtr<ID3D12Device> _dev)
{
	SetMutipassSRVHeapDesc();

	return _dev->CreateDescriptorHeap
	(
		&mutipassSRVHeapDesc,
		IID_PPV_ARGS(multipassSRVHeap.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfVertex(ComPtr<ID3D12Device> _dev)
{
	SetVertexHeapProp();
	vertresDesc = CD3DX12_RESOURCE_DESC::Buffer(pmdMaterialInfo->vertices.size());
	
	return _dev->CreateCommittedResource
	(
		&vertexHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfIndex(ComPtr<ID3D12Device> _dev)
{
	indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size());

	return _dev->CreateCommittedResource
	(
		&vertexHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfDepthAndLightMap(ComPtr<ID3D12Device> _dev)
{
	// �f�v�X�}�b�v�o�b�t�@�[�쐬
	SetDepthHeapProp();
	SetDepthResourceDesc();
	D3D12_CLEAR_VALUE depthClearValue2 = {};
	depthClearValue2.DepthStencil.Depth = 1.0f;
	depthClearValue2.Format = DXGI_FORMAT_D32_FLOAT;

	_dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&/*depthResDesc*/depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue2,
		IID_PPV_ARGS(depthBuff.ReleaseAndGetAddressOf())
	);

	_dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&/*depthResDesc*/depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue2,
		IID_PPV_ARGS(depthBuff2.ReleaseAndGetAddressOf())
	);

	// ���C�g�}�b�v�o�b�t�@�[�쐬
	lightMapHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	lightMapHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	lightMapHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	constexpr uint32_t shadow_difinition = 1024;
	lightMapResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	lightMapResDesc.Width = shadow_difinition;
	lightMapResDesc.Height = shadow_difinition;
	lightMapResDesc.DepthOrArraySize = 1;
	lightMapResDesc.Format = DXGI_FORMAT_R32_TYPELESS; // �[�x�l�������ݗp
	lightMapResDesc.SampleDesc.Count = 1; // 1pixce/1�̃T���v��
	lightMapResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	return _dev->CreateCommittedResource
	(
		&lightMapHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&lightMapResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue2,
		IID_PPV_ARGS(lightMapBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev)
{
	// ��ʓI��WVP(���f����world���r���[���v���W�F�N�V����)�ϊ����s���̂ŁA���p�\�ȃn�Y...
	wvpHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	wvpResdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);

	_dev->CreateCommittedResource
	(
		&wvpHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&wvpResdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(matrixBuff.ReleaseAndGetAddressOf())
	);

	return 	_dev->CreateCommittedResource
	(
		&wvpHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&wvpResdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(matrixBuff4Multipass.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateConstBufferOfMaterial(ComPtr<ID3D12Device> _dev)
{
	materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	materialBuffResDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * pmdMaterialInfo->materialNum);

	return _dev->CreateCommittedResource
	(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialBuffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateRenderBufferForMultipass(ComPtr<ID3D12Device> _dev, D3D12_RESOURCE_DESC& mutipassResDesc)
{
	mutipassHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	SetClearValue();

	result = _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue,
		IID_PPV_ARGS(multipassBuff.ReleaseAndGetAddressOf())
	);

	result = _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue,
		IID_PPV_ARGS(multipassBuff2.ReleaseAndGetAddressOf())
	);

	result = _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue,
		IID_PPV_ARGS(multipassBuff3.ReleaseAndGetAddressOf())
	);

	// for bloom[3]
	for (auto& res : _bloomBuff)
	{
		result = _dev->CreateCommittedResource
		(
			&mutipassHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&mutipassResDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&depthClearValue,
			IID_PPV_ARGS(res.ReleaseAndGetAddressOf())
		);
	}

	// for AO
	D3D12_CLEAR_VALUE depthClearValue4AO = depthClearValue;
	depthClearValue4AO.Format = DXGI_FORMAT_R32_FLOAT;
	mutipassResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	result = _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue4AO,
		IID_PPV_ARGS(aoBuff.ReleaseAndGetAddressOf())
	);

	mutipassResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	result = _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue,
		IID_PPV_ARGS(imguiBuff.ReleaseAndGetAddressOf())
	);

	return result;

}

HRESULT BufferHeapCreator::CreateConstBufferOfGaussian(ComPtr<ID3D12Device> _dev, std::vector<float> weights)
{
	gaussianHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	gaussianBuffResDesc = CD3DX12_RESOURCE_DESC::Buffer(Utility::AlignmentSize(sizeof(weights[0]) * weights.size(), 256));
	return _dev->CreateCommittedResource
	(
		&gaussianHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&gaussianBuffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(gaussianBuff.ReleaseAndGetAddressOf())
	);
}

void BufferHeapCreator::CreateUploadAndReadBuff4Normalmap(ComPtr<ID3D12Device> _dev,
	std::string strModelPath,
	std::string fileType, 
	unsigned int texNum)
{
	// TODO:�ėp����������B���f���p�X�ƈȉ��f�[�^��map���쐬���ă��f�����ɊǗ�������
	// ���̂悤�Ƀo�b�t�@�[�Ȃǈꊇ�Ń��T�C�Y������@���D�܂���
	normalMapUploadBuff.resize(texNum);
	normalMapReadBuff.resize(texNum);
	normalMapMetaData.resize(texNum);
	normalMapImg.resize(texNum);

	for (int i = 0; i < texNum; i++)
	{
		filePath = "";

		//�g�D�[�����\�[�X�̓ǂݍ���
		char fileName[32];
		sprintf(fileName, "texture\\normal%d.%s", i + 1, fileType.c_str());
		filePath += fileName;
		filePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, filePath.c_str());

		auto wTexPath = Utility::GetWideStringFromSring(filePath);
		auto extention = Utility::GetExtension(filePath);

		if (!textureLoader->GetTable().count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			//return 0;
			break;
		}

		normalMapMetaData[i] = new TexMetadata;
		result = textureLoader->GetTable()[extention](wTexPath, normalMapMetaData[i], normalMapScratchImg);

		if (normalMapScratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector �̌^��const�K�p����ƃR���p�C���ɂ�苓�����ω����邽�ߋ֎~
		normalMapImg[i] = new Image;
		normalMapImg[i]->pixels = normalMapScratchImg.GetImage(0, 0, 0)->pixels;
		normalMapImg[i]->rowPitch = normalMapScratchImg.GetImage(0, 0, 0)->rowPitch;
		normalMapImg[i]->format = normalMapScratchImg.GetImage(0, 0, 0)->format;
		normalMapImg[i]->width = normalMapScratchImg.GetImage(0, 0, 0)->width;
		normalMapImg[i]->height = normalMapScratchImg.GetImage(0, 0, 0)->height;
		normalMapImg[i]->slicePitch = normalMapScratchImg.GetImage(0, 0, 0)->slicePitch;

		// �m�[�}���}�b�v�����݂���ꍇ
		if (_stat(filePath.c_str(), &s) == 0)
		{
			std::tie(normalMapUploadBuff[i], normalMapReadBuff[i]) = CreateD3DX12ResourceBuffer::LoadTextureFromFile(_dev, normalMapMetaData[i], normalMapImg[i], filePath);
		}

		else
			std::tie(normalMapUploadBuff[i], normalMapReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
	}
}

void BufferHeapCreator::CreateUploadAndReadBuff4PmdTexture(ComPtr<ID3D12Device> _dev,
	std::string strModelPath, std::vector<DirectX::TexMetadata*>& metaData, std::vector<DirectX::Image*>& img)
{
	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	for (int i = 0; i < pmdMaterialInfo->materials.size(); i++)
	{
		if (strlen(pmdMaterialInfo->materials[i].addtional.texPath.c_str()) == 0)
		{
			pmdTexUploadBuff[i] = nullptr;
			pmdTexReadBuff[i] = nullptr;
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
			std::tie(pmdTexUploadBuff[i], pmdTexReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			spaMappedBuff[i] = nullptr;
		}

		else if (Utility::GetExtension(texFileName) == "spa")
		{
			spaMappedBuff[i] = CreateD3DX12ResourceBuffer::CreateMappedSphSpaTexResource(_dev, metaData[i], img[i], texFilePath);
			std::tie(pmdTexUploadBuff[i], pmdTexReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			sphMappedBuff[i] = nullptr;
		}

		else
		{
			std::tie(pmdTexUploadBuff[i], pmdTexReadBuff[i]) = CreateD3DX12ResourceBuffer::LoadTextureFromFile(_dev, metaData[i], img[i], texFilePath);
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
		toonFilePath = "";

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
		toonImg[i]->pixels = toonScratchImg.GetImage(0, 0, 0)->pixels;
		toonImg[i]->rowPitch = toonScratchImg.GetImage(0, 0, 0)->rowPitch;
		toonImg[i]->format = toonScratchImg.GetImage(0, 0, 0)->format;
		toonImg[i]->width = toonScratchImg.GetImage(0, 0, 0)->width;
		toonImg[i]->height = toonScratchImg.GetImage(0, 0, 0)->height;
		toonImg[i]->slicePitch = toonScratchImg.GetImage(0, 0, 0)->slicePitch;

		// tooIdx�w��(+1)��toon�t�@�C�������݂���ꍇ
		if (_stat(toonFilePath.c_str(), &s) == 0)
		{
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = CreateD3DX12ResourceBuffer::LoadTextureFromFile(_dev, toonMetaData[i], toonImg[i], toonFilePath);
		}

		else
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
	}
}

void BufferHeapCreator::CreateTextureBuffers(ComPtr<ID3D12Device> _dev)
{
	whiteTextureBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0xff);
	blackTextureBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0x00);
	grayTextureBuff = CreateD3DX12ResourceBuffer::CreateGrayGradationTexture(_dev);
}

HRESULT BufferHeapCreator::CreateBuff4Imgui(ComPtr<ID3D12Device> _dev, size_t sizeofPostSetting)
{
	auto bufferSize = Utility::AlignmentSize(sizeofPostSetting, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffResDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	HRESULT result;

	result = _dev->CreateCommittedResource
	(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&buffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�q�[�v�ł̃��\�[�X������Ԃ͂��̃^�C�v���������[��
		nullptr,
		IID_PPV_ARGS(imguiPostSettingBuff.ReleaseAndGetAddressOf())
	);

	//if (result != S_OK)
	//{
	//	return result;
	//}

	//D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//desc.NodeMask = 0;
	//desc.NumDescriptors = 1;
	//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//result = _dev->CreateDescriptorHeap
	//(
	//	&desc, IID_PPV_ARGS(imguiPostSettingHeap.GetAddressOf())
	//);

	return result;
}