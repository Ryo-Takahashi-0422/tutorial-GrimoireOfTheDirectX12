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

	texUploadBuff.resize(pmdMaterialInfo->materialNum);//テクスチャCPUアップロード用バッファー
	texReadBuff.resize(pmdMaterialInfo->materialNum);//テクスチャGPU読み取り用バッファー
	sphMappedBuff.resize(pmdMaterialInfo->materialNum);//sph用バッファー
	spaMappedBuff.resize(pmdMaterialInfo->materialNum);//spa用バッファー
	toonUploadBuff.resize(pmdMaterialInfo->materialNum);//トゥーン用アップロードバッファー
	toonReadBuff.resize(pmdMaterialInfo->materialNum);//トゥーン用リードバッファー

	materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;
}

void BufferHeapCreator::SetRTVHeapDesc()
{
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
}

void BufferHeapCreator::SetDSVHeapDesc()
{
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
}

void BufferHeapCreator::SetCBVSRVHeapDesc()
{
	cbvsrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvsrvHeapDesc.NumDescriptors = 1 + pmdMaterialInfo->materialNum * 5; // 行列cbv + (material cbv+テクスチャsrv+sph srv+spa srv+toon srv)
	cbvsrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvsrvHeapDesc.NodeMask = 0;
}

void BufferHeapCreator::SetMutipassRTVHeapDesc()
{
	mutipassRTVHeapDesc = rtvHeaps->GetDesc(); // 既存のヒープから設定継承
	mutipassRTVHeapDesc.NumDescriptors = 2; // マルチパス対象数で変動する
}

void BufferHeapCreator::SetMutipassSRVHeapDesc()
{
	mutipassSRVHeapDesc = rtvHeaps->GetDesc(); // 既存のヒープから設定継承
	mutipassSRVHeapDesc.NumDescriptors = 3; // マルチパス対象数で変動する + effect
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
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用
	depthResDesc.SampleDesc.Count = 1; // 1pixce/1つのサンプル
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
}

void BufferHeapCreator::SetClearValue()
{
	float clsClr[4] = { 0.5,0.5,0.5,1.0 };
	depthClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);
	//depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	//depthClearValue.DepthStencil.Depth = 1.0f; // 深さ1.0(最大値)でクリア

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
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadヒープでのリソース初期状態はこのタイプが公式ルール
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
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadヒープでのリソース初期状態はこのタイプが公式ルール
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf())
	);
}

HRESULT BufferHeapCreator::CreateBufferOfDepth(ComPtr<ID3D12Device> _dev)
{
	SetDepthHeapProp();
	SetDepthResourceDesc();

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

HRESULT BufferHeapCreator::CreateConstBufferOfWVPMatrix(ComPtr<ID3D12Device> _dev)
{
	// 一般的なWVP(モデル→world→ビュー→プロジェクション)変換を行うので、流用可能なハズ...
	wvpHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	wvpResdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);

	return _dev->CreateCommittedResource
	(
		&wvpHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&wvpResdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadヒープでのリソース初期状態はこのタイプが公式ルール
		nullptr,
		IID_PPV_ARGS(matrixBuff.ReleaseAndGetAddressOf())
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
		D3D12_RESOURCE_STATE_GENERIC_READ, // Uploadヒープでのリソース初期状態はこのタイプが公式ルール
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

	return _dev->CreateCommittedResource
	(
		&mutipassHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&mutipassResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClearValue,
		IID_PPV_ARGS(multipassBuff2.ReleaseAndGetAddressOf())
	);
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

void BufferHeapCreator::CreateUploadAndReadBuff(ComPtr<ID3D12Device> _dev,
	std::string strModelPath, std::vector<DirectX::TexMetadata*>& metaData, std::vector<DirectX::Image*>& img)
{
	// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	for (int i = 0; i < pmdMaterialInfo->materials.size(); i++)
	{
		if (strlen(pmdMaterialInfo->materials[i].addtional.texPath.c_str()) == 0)
		{
			texUploadBuff[i] = nullptr;
			texReadBuff[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterialInfo->materials[i].addtional.texPath;

		// ファイル名に*を含む場合の処理
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

		// spa,sph拡張子ファイルはslicepitchが大きすぎてオーバーフロー?するため、バッファー作成に失敗する。
		// 更に詳細は不明だがこれによりなぜかbufferHeapCreator->GetVertBuff()のマッピングが失敗するようになるため、一時回避する

		auto texFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, texFileName.c_str());
		auto wTexPath = Utility::GetWideStringFromSring(texFilePath);
		auto extention = Utility::GetExtension(texFilePath);

		if (!textureLoader->GetTable().count(extention))
		{
			std::cout << "読み込めないテクスチャが存在します" << std::endl;
			return;
		}
		metaData[i] = new TexMetadata;
		result = textureLoader->GetTable()[extention](wTexPath, metaData[i], scratchImg);

		if (scratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector の型にconst適用するとコンパイラにより挙動が変化するため禁止
		img[i] = new Image;
		img[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		img[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		img[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		img[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		img[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		img[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// CPU主導でGPUへsphファイルのバッファ生成・サブリソースへコピー
		// 要リファクタリング

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
		//トゥーンリソースの読み込み
		char toonFileName[16];
		sprintf(toonFileName, "toon%02d.bmp", pmdMaterialInfo->materials[i].addtional.toonIdx + 1);
		toonFilePath += toonFileName;
		toonFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, toonFilePath.c_str());

		auto wTexPath = Utility::GetWideStringFromSring(toonFilePath);
		auto extention = Utility::GetExtension(toonFilePath);

		if (!textureLoader->GetTable().count(extention))
		{
			std::cout << "読み込めないテクスチャが存在します" << std::endl;
			//return 0;
			break;
		}

		toonMetaData[i] = new TexMetadata;
		result = textureLoader->GetTable()[extention](wTexPath, toonMetaData[i], toonScratchImg);

		if (toonScratchImg.GetImage(0, 0, 0) == nullptr) continue;

		// std::vector の型にconst適用するとコンパイラにより挙動が変化するため禁止
		toonImg[i] = new Image;
		toonImg[i]->pixels = scratchImg.GetImage(0, 0, 0)->pixels;
		toonImg[i]->rowPitch = scratchImg.GetImage(0, 0, 0)->rowPitch;
		toonImg[i]->format = scratchImg.GetImage(0, 0, 0)->format;
		toonImg[i]->width = scratchImg.GetImage(0, 0, 0)->width;
		toonImg[i]->height = scratchImg.GetImage(0, 0, 0)->height;
		toonImg[i]->slicePitch = scratchImg.GetImage(0, 0, 0)->slicePitch;

		// tooIdx指定(+1)のtoonファイルが存在する場合
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