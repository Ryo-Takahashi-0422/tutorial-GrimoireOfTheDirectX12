#include <stdafx.h>
#include <AppD3DX12.h>

#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

AppD3DX12& AppD3DX12::Instance()
{
	static AppD3DX12 instance;
	return instance;
};

// 後処理
void AppD3DX12::Terminate()
{

};

AppD3DX12::~AppD3DX12()
{

};

HRESULT AppD3DX12::D3DX12DeviceInit()
{
	result = CoInitializeEx(0, COINIT_MULTITHREADED);

	//ファクトリーの生成
	result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()))))
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()))))
		{
			return -1;
		}
	}

	//グラボが複数挿入されている場合にアダプターを選択するための処理
	//アダプターの列挙用
	std::vector <IDXGIAdapter*> adapters;
	//特定の名前を持つアダプターオブジェクトが入る
	ComPtr<IDXGIAdapter> tmpAdapter = nullptr;

	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, tmpAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		adapters.push_back(tmpAdapter.Get());
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); //アダプターの説明オブジェクト取得

		//探したいアダプターの名前を確認
		std::wstring strDesc = adesc.Description;
		//printf("%ls\n", strDesc.c_str());
		//printf("%x\n", adesc.DeviceId);

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//Direct3D デバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter.Get(), lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;//生成可能なバージョンが見つかったらループ中断
		}
	}

	_fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
}

//#ifdef _DEBUG
bool AppD3DX12::PrepareRendering() {
//#else
//int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//{
//#endif

	//strModelPath[0] = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd";
	//strModelPath[1] = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\鏡音レン.pmd";
	strModelPath =
	{
		"C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\初音ミク.pmd",
		"C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\鏡音レン.pmd",
		"C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\弱音ハク.pmd"
	};

	strModelNum = strModelPath.size();

	// SetRootSignatureBaseクラスのインスタンス化
	setRootSignature = new SetRootSignature;

	// SettingShaderCompileクラスのインスタンス化
	settingShaderCompile = new SettingShaderCompile;

	// VertexInputLayoutクラスのインスタンス化
	vertexInputLayout = new VertexInputLayout;
	
	// PMDファイルの読み込み
	pmdMaterialInfo.resize(strModelNum);

	for (int i = 0; i < strModelNum; ++i)
	{
		pmdMaterialInfo[i] = new PMDMaterialInfo;
		if (FAILED(pmdMaterialInfo[i]->ReadPMDHeaderFile(strModelPath[i]))) return false;
	}

	// VMDモーションファイルの読み込み
	vmdMotionInfo.resize(strModelNum);
	for (int i = 0; i < strModelNum; ++i)
	{
		vmdMotionInfo[i] = new VMDMotionInfo;
		if (FAILED(vmdMotionInfo[i]->ReadVMDHeaderFile(strMotionPath))) return false;
	}

	// PMDActorクラスのインスタンス化
	pmdActor.resize(strModelNum);// = new PMDActor(pmdMaterialInfo, vmdMotionInfo);
	for (int i = 0; i < strModelNum; ++i)
	{

		pmdActor[i] = new PMDActor(pmdMaterialInfo[i], vmdMotionInfo[i]);
	}

	// GraphicsPipelineSettingクラスのインスタンス化
	gPLSetting = new GraphicsPipelineSetting(vertexInputLayout);

	// アニメーション用の回転・並行移動行列の参照準備
	for (int i = 0; i < strModelNum; ++i)
	{
		boneMatrices[i] = pmdActor[i]->GetMatrices();
		bNodeTable[i] = pmdMaterialInfo[i]->GetBoneNode();
	}
	
	// レンダリングウィンドウ設定
	prepareRenderingWindow = new PrepareRenderingWindow;
	prepareRenderingWindow->CreateAppWindow();

	// TextureLoaderクラスのインスタンス化
	textureLoader = new TextureLoader;

	bufferHeapCreator.resize(strModelNum);
	textureTransporter.resize(strModelNum);
	mappingExecuter.resize(strModelNum);
	viewCreator.resize(strModelNum);

	// アニメーション用の回転・並行移動行列の参照準備
	for (int i = 0; i < strModelNum; ++i)
	{
		// BufferHeapCreatorクラスのインスタンス化
		bufferHeapCreator[i] = new BufferHeapCreator(pmdMaterialInfo[i], prepareRenderingWindow, textureLoader);

		// TextureTransporterクラスのインスタンス化
		textureTransporter[i] = new TextureTransporter(pmdMaterialInfo[i], bufferHeapCreator[i]);

		// MappingExecuterクラスのインスタンス化
		mappingExecuter[i] = new MappingExecuter(pmdMaterialInfo[i], bufferHeapCreator[i]);

		// ViewCreatorクラスのインスタンス化
		viewCreator[i] = new ViewCreator(pmdMaterialInfo[i], bufferHeapCreator[i]);
	}
	// レンダリングウィンドウ表示
	ShowWindow(prepareRenderingWindow->GetHWND(), SW_SHOW);

	// ビューポートとシザー領域の設定
	prepareRenderingWindow->SetViewportAndRect();

	// ﾏﾙﾁﾊﾟｽ関連ｸﾗｽ群
	peraLayout = new PeraLayout;
	peraGPLSetting = new PeraGraphicsPipelineSetting(peraLayout/*vertexInputLayout*/); //TODO PeraLayout,VertexInputLayoutｸﾗｽの基底クラスを作ってそれに対応させる
	peraPolygon = new PeraPolygon;
	peraSetRootSignature = new PeraSetRootSignature;
	peraShaderCompile = new PeraShaderCompile;
	bufferGPLSetting = new PeraGraphicsPipelineSetting(peraLayout/*vertexInputLayout*/);
	bufferSetRootSignature = new PeraSetRootSignature;
	bufferShaderCompile = new BufferShaderCompile;

	// ライトマップ関連
	lightMapGPLSetting = new LightMapGraphicsPipelineSetting(vertexInputLayout);
	lightMapRootSignature = new SetRootSignature;
	lightMapShaderCompile = new LightMapShaderCompile;

	// bloom	
	bloomGPLSetting = new PeraGraphicsPipelineSetting(peraLayout);
	bloomRootSignature = new PeraSetRootSignature;
	bloomShaderCompile = new BloomShaderCompile;

	//AO
	aoGPLSetting = new AOGraphicsPipelineSetting(vertexInputLayout);
	aoRootSignature = new SetRootSignature;
	aoShaderCompile = new AOShaderCompile;
}

bool AppD3DX12::PipelineInit(){
//●パイプライン初期化　処理１～７
//初期化処理１：デバッグレイヤーをオンに
#ifdef _DEBUG
	Utility::EnableDebugLayer();
#endif
//初期化処理２：各種デバイスの初期設定
	D3DX12DeviceInit();

	// カラークリアに関するWarningをフィルタリング(メッセージが埋もれてしまう...)
	_dev.As(&infoQueue);

	D3D12_MESSAGE_ID denyIds[] = 
	{
	  D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE ,
	};
	D3D12_MESSAGE_SEVERITY severities[] = 
	{
	  D3D12_MESSAGE_SEVERITY_INFO
	};
	D3D12_INFO_QUEUE_FILTER filter{};
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;
	filter.DenyList.NumSeverities = _countof(severities);
	filter.DenyList.pSeverityList = severities;

	if (infoQueue != nullptr)
	{
		result = infoQueue->PushStorageFilter(&filter);
		// ついでにエラーメッセージでブレークさせる
		result = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	}

//初期化処理３：コマンドキューの記述用意・作成

		//コマンドキュー生成、詳細obj生成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウト無し
	cmdQueueDesc.NodeMask = 0;//アダプターを一つしか使わないときは0でOK
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//コマンドキューの優先度
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//コマンドリストと合わせる

	//コマンドキュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));

//初期化処理４：スワップチェーンの生成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = prepareRenderingWindow->GetWindowWidth();
	swapChainDesc.Height = prepareRenderingWindow->GetWindowHeight();
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	result = _dxgiFactory->CreateSwapChainForHwnd( //ここで生成
		_cmdQueue.Get(),
		prepareRenderingWindow->GetHWND(),
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf());

//初期化処理５：レンダーターゲットビュー(RTV)の記述子ヒープを作成
	for (int i = 0; i < strModelNum; ++i)
	{
		//RTV 記述子ヒープ領域の確保
		bufferHeapCreator[i]->SetRTVHeapDesc();
		//RTV用記述子ヒープの生成　ID3D12DescriptorHeap：記述子の連続したコレクション
		result = bufferHeapCreator[i]->CreateRTVHeap(_dev);
	}
	//以下のように記述することでスワップチェーンの持つ情報を新たなDescオブジェクトにコピーできる
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//スワップチェーンの説明
	//result = _swapChain->GetDesc(&swcDesc);//SWCの説明を取得する

//初期化処理６：フレームリソース(各フレームのレンダーターゲットビュー)を作成
	_backBuffers.resize(swapChainDesc.BufferCount); // ｽﾜｯﾌﾟﾁｪｰﾝﾊﾞｯｸﾊﾞｯﾌｧｰのﾘｻｲｽﾞ

	handle = bufferHeapCreator[0]->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();//ヒープの先頭を表す CPU 記述子ハンドルを取得

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM/*_SRGB*/;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swapChainDesc.BufferCount; idx++)
	{   //swapEffect DXGI_SWAP_EFFECT_FLIP_DISCARD の場合は最初のバッファーのみアクセス可能
		result = _swapChain->GetBuffer(idx, IID_PPV_ARGS(_backBuffers[idx].ReleaseAndGetAddressOf()));//SWCにバッファーのIIDとそのIIDポインタを教える(SWCがレンダリング時にアクセスする)

		_dev->CreateRenderTargetView//リソースデータ(_backBuffers)にアクセスするためのレンダーターゲットビューをhandleアドレスに作成
		(
			_backBuffers[idx].Get(),//レンダーターゲットを表す ID3D12Resource オブジェクトへのポインター
			&rtvDesc,//レンダー ターゲット ビューを記述する D3D12_RENDER_TARGET_VIEW_DESC 構造体へのポインター。
			handle//新しく作成されたレンダーターゲットビューが存在する宛先を表す CPU 記述子ハンドル(ヒープ上のアドレス)
		);

		//handleｱﾄﾞﾚｽを記述子のアドレスを扱う記述子サイズ分オフセットしていく
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

//初期化処理７：コマンドアロケーターを作成
			//コマンドアロケーター生成>>コマンドリスト作成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	return true;
}

bool AppD3DX12::ResourceInit() {
	//●リソース初期化

	// 初期化処理1：ルートシグネチャ設定
	if (FAILED(setRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// ﾏﾙﾁﾊﾟｽ用
	if (FAILED(peraSetRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// 表示用
	if (FAILED(bufferSetRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// ライトマップ用
	if (FAILED(lightMapRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// bloom
	if (FAILED(bloomRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// AO
	if (FAILED(aoRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}
	
// 初期化処理2：シェーダーコンパイル設定
	// _vsBlobと_psBlobにｼｪｰﾀﾞｰｺﾝﾊﾟｲﾙ設定を割り当てる。それぞれﾌｧｲﾙﾊﾟｽを保持するが読み込み失敗したらnullptrが返ってくる。
	auto blobs = settingShaderCompile->SetShaderCompile(setRootSignature, _vsBlob, _psBlob);
	if (blobs.first == nullptr or blobs.second == nullptr) return false;
	_vsBlob = blobs.first;
	_psBlob = blobs.second;	

	// ﾏﾙﾁﾊﾟｽ1枚目用
	auto mBlobs = peraShaderCompile->SetPeraShaderCompile(peraSetRootSignature, _vsMBlob, _psMBlob);
	if (mBlobs.first == nullptr or mBlobs.second == nullptr) return false;
	_vsMBlob = mBlobs.first;
	_psMBlob = mBlobs.second;

	// 表示用
	auto bufferBlobs = bufferShaderCompile->SetPeraShaderCompile(bufferSetRootSignature, _vsBackbufferBlob, _psBackbufferBlob);
	if (bufferBlobs.first == nullptr or bufferBlobs.second == nullptr) return false;
	_vsBackbufferBlob = bufferBlobs.first;
	_psBackbufferBlob = bufferBlobs.second;

	// ライトマップ用
	auto lightMapBlobs = lightMapShaderCompile->SetShaderCompile(lightMapRootSignature, _lightMapVSBlob, _lightMapPSBlob);
	if (lightMapBlobs.first == nullptr) return false;
	_lightMapVSBlob = lightMapBlobs.first;
	_lightMapPSBlob = lightMapBlobs.second; // こちらはnullptr

	// bloom
	auto bloomBlobs = bloomShaderCompile->SetPeraShaderCompile(bloomRootSignature, _bloomVSBlob, _bloomPSBlob);
	if (bloomBlobs.first == nullptr or bufferBlobs.second == nullptr) return false;
	_bloomVSBlob = bloomBlobs.first; // こちらはnullpt
	_bloomPSBlob = bloomBlobs.second;

	// AO
	auto aoBlobs = aoShaderCompile->SetShaderCompile(aoRootSignature, _aoVSBlob, _aoPSBlob);
	if (aoBlobs.first == nullptr or aoBlobs.second == nullptr) return false;
	_aoVSBlob = aoBlobs.first; // こちらはnullpt
	_aoPSBlob = aoBlobs.second;

// 初期化処理3：頂点入力レイアウトの作成及び
// 初期化処理4：パイプライン状態オブジェクト(PSO)のDesc記述してオブジェクト作成
	result = gPLSetting->CreateGPStateWrapper(_dev, setRootSignature, _vsBlob, _psBlob);

	// ﾏﾙﾁﾊﾟｽ用
	result = peraGPLSetting->CreateGPStateWrapper(_dev, peraSetRootSignature, _vsMBlob, _psMBlob);

	// 表示用
	result = bufferGPLSetting->CreateGPStateWrapper(_dev, bufferSetRootSignature, _vsBackbufferBlob, _psBackbufferBlob);

	// ライトマップ用
	result = lightMapGPLSetting->CreateGPStateWrapper(_dev, lightMapRootSignature, _lightMapVSBlob, _lightMapPSBlob);

	// for shrinked bloom creating
	result = bloomGPLSetting->CreateGPStateWrapper(_dev, bloomRootSignature, _bloomVSBlob, _bloomPSBlob);

	// AO
	result = aoGPLSetting->CreateGPStateWrapper(_dev, aoRootSignature, _aoVSBlob, _aoPSBlob);

// 初期化処理5：コマンドリスト生成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

// 初期化処理6：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
	//cmdList->Close();

// 初期化処理7：各バッファーを作成して頂点情報を読み込み
	for (int i = 0; i < strModelNum; ++i)
	{
		//頂点バッファーの作成(リソースと暗黙的なヒープの作成) 
		result = bufferHeapCreator[i]->CreateBufferOfVertex(_dev);

		//インデックスバッファーを作成(リソースと暗黙的なヒープの作成)
		result = bufferHeapCreator[i]->CreateBufferOfIndex(_dev);

		//デプスバッファーを作成
		result = bufferHeapCreator[i]->CreateBufferOfDepthAndLightMap(_dev);

		//ファイル形式毎のテクスチャロード処理
		textureLoader->LoadTexture();

		// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
		metaData.resize(pmdMaterialInfo[i]->materialNum);
		img.resize(pmdMaterialInfo[i]->materialNum);
		ScratchImage scratchImg = {};
		//for (auto& path : strModelPath)
		//{
		bufferHeapCreator[i]->CreateUploadAndReadBuff4PmdTexture(_dev, strModelPath[i], metaData, img); // バッファ作成
		//}

		// トゥーンテクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
		toonMetaData.resize(pmdMaterialInfo[i]->materialNum);
		toonImg.resize(pmdMaterialInfo[i]->materialNum);
		//for (auto& path : strModelPath)
		//{
		bufferHeapCreator[i]->CreateToonUploadAndReadBuff(_dev, strModelPath[i], toonMetaData, toonImg); // バッファ作成
		//}

		//行列用定数バッファーの生成
		pmdMaterialInfo[i]->worldMat = XMMatrixIdentity();

		//auto worldMat = XMMatrixRotationY(15.0f);
		pmdMaterialInfo[i]->angle = 0.0f;

		//ビュー行列の生成・乗算
		XMFLOAT3 eye(0, 15, -15);
		XMFLOAT3 target(0, 10, 0);
		XMFLOAT3 up(0, 1, 0);
		auto viewMat = XMMatrixLookAtLH
		(
			XMLoadFloat3(&eye),
			XMLoadFloat3(&target),
			XMLoadFloat3(&up)
		);

		//プロジェクション(射影)行列の生成・乗算
		auto projMat = XMMatrixPerspectiveFovLH
		(
			XM_PIDIV2, // 画角90°
			static_cast<float>(prepareRenderingWindow->GetWindowHeight()) / static_cast<float>(prepareRenderingWindow->GetWindowWidth()),
			1.0, // ニア―クリップ
			100.0 // ファークリップ
		);

		// 行列用定数バッファーの生成
		result = bufferHeapCreator[i]->CreateConstBufferOfWVPMatrix(_dev);

		//マテリアル用定数バッファーの生成
		result = bufferHeapCreator[i]->CreateConstBufferOfMaterial(_dev);

		// マルチパスレンダリング用に書き込み先リソースの作成
		// 作成済みのヒープ情報を使ってもう一枚レンダリング先を用意
		// 使っているバックバッファーの情報を利用する
		auto& bbuff = _backBuffers[0];
		auto mutipassResDesc = bbuff->GetDesc();
		// RTV,SRV用バッファーと各ヒープ作成
		result = bufferHeapCreator[i]->CreateRenderBufferForMultipass(_dev, mutipassResDesc);
		bufferHeapCreator[i]->CreateMultipassRTVHeap(_dev);
		bufferHeapCreator[i]->CreateMultipassSRVHeap(_dev);

		//頂点バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的に頂点データをコピーする。
		mappingExecuter[i]->MappingVertBuff();

		//インデクスバッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
		mappingExecuter[i]->MappingIndexOfVertexBuff();

		//行列用定数バッファーのマッピング
		// 平面、ライト座標
		_planeNormalVec = XMFLOAT4(0, 1, 0, 0);
		lightVec = XMFLOAT3(-0.5f, 1, -0.5f);
		auto light = XMLoadFloat3(&lightVec);
		auto eyePos = XMLoadFloat3(&eye);
		auto targetPos = XMLoadFloat3(&target);
		auto upVec = XMLoadFloat3(&up);
		light = targetPos + XMVector3Normalize(light) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
		XMVECTOR det;

		result = bufferHeapCreator[i]->GetMatrixBuff()->Map(0, nullptr, (void**)&pmdMaterialInfo[i]->mapMatrix);
		pmdMaterialInfo[i]->mapMatrix->world = pmdMaterialInfo[i]->worldMat;
		pmdMaterialInfo[i]->mapMatrix->view = viewMat;
		pmdMaterialInfo[i]->mapMatrix->proj = projMat;
		pmdMaterialInfo[i]->mapMatrix->invProj = XMMatrixInverse(&det, projMat);
		pmdMaterialInfo[i]->mapMatrix->lightCamera = XMMatrixLookAtLH(light, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);
		pmdMaterialInfo[i]->mapMatrix->shadow = XMMatrixShadow(XMLoadFloat4(&_planeNormalVec), -XMLoadFloat3(&lightVec));
		pmdMaterialInfo[i]->mapMatrix->eye = eye;

		// ↑とほぼ同じことをしている。ライトマップ用にもmapMatrixにマッピングしたらモデルが消える。TODO:いけてないのでなんとかしたい...
		
		result = bufferHeapCreator[i]->GetMatrixBuff4Multipass()->Map(0, nullptr, (void**)&pmdMaterialInfo[i]->mapMatrix4Lightmap); // ライトマップ用にもマッピング
		pmdMaterialInfo[i]->mapMatrix4Lightmap->world = pmdMaterialInfo[i]->worldMat;
		pmdMaterialInfo[i]->mapMatrix4Lightmap->view = viewMat;
		pmdMaterialInfo[i]->mapMatrix4Lightmap->proj = projMat;
		pmdMaterialInfo[i]->mapMatrix4Lightmap->invProj = XMMatrixInverse(&det, projMat);
		pmdMaterialInfo[i]->mapMatrix4Lightmap->lightCamera = XMMatrixLookAtLH(light, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);
		pmdMaterialInfo[i]->mapMatrix4Lightmap->shadow = XMMatrixShadow(XMLoadFloat4(&_planeNormalVec), -XMLoadFloat3(&lightVec));
		pmdMaterialInfo[i]->mapMatrix4Lightmap->eye = eye;

		//マテリアル用バッファーへのマッピング
		mappingExecuter[i]->MappingMaterialBuff();

		// TODO:strMoodelPathをリスト化してまとめて処理出来るようにする。BufferHeapCreatorにMappngExecuterを握らせる。
		// ノーマルマップ読み込み、バッファ作成、マッピング
		//for (auto& path : strModelPath)
		//{
		bufferHeapCreator[i]->CreateUploadAndReadBuff4Normalmap(_dev, strModelPath[i], "jpg", 1);
		//}

		mappingExecuter[i]->TransferTexUploadToBuff(bufferHeapCreator[i]->GetNormalMapUploadBuff(), bufferHeapCreator[i]->GetNormalMapImg(), 1);
		textureTransporter[i]->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue,
			bufferHeapCreator[i]->GetNormalMapMetadata(), bufferHeapCreator[i]->GetNormalMapImg(),
			_fence, _fenceVal, bufferHeapCreator[i]->GetNormalMapUploadBuff(), bufferHeapCreator[i]->GetNormalMapReadBuff(), 1);

		// テクスチャのアップロード用バッファへのマッピング
		mappingExecuter[i]->TransferTexUploadToBuff(bufferHeapCreator[i]->GetPMDTexUploadBuff(), img, pmdMaterialInfo[i]->materialNum);
		// テクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
		textureTransporter[i]->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue, metaData, img,
			_fence, _fenceVal, bufferHeapCreator[i]->GetPMDTexUploadBuff(), bufferHeapCreator[i]->GetPMDTexReadBuff(), pmdMaterialInfo[i]->materialNum);

		// トゥーンテクスチャも同様にマッピング
		mappingExecuter[i]->TransferTexUploadToBuff(bufferHeapCreator[i]->GetToonUploadBuff(), toonImg, pmdMaterialInfo[i]->materialNum);
		// トゥーンテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
		textureTransporter[i]->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue, toonMetaData, toonImg,
			_fence, _fenceVal, bufferHeapCreator[i]->GetToonUploadBuff(), bufferHeapCreator[i]->GetToonReadBuff(), pmdMaterialInfo[i]->materialNum);

		//CBV,SRVディスクリプタヒープ作成(行列、テクスチャに利用)
		result = bufferHeapCreator[i]->CreateCBVSRVHeap(_dev);

		//DSVビュー用にディスクリプタヒープ作成
		result = bufferHeapCreator[i]->CreateDSVHeap(_dev);

		// 初期化処理8：各ビューを作成

			// Vertexビュー作成
		viewCreator[i]->CreateVertexBufferView();

		// Indexビュー作成
		viewCreator[i]->CreateIndexBufferView();

		// DSV作成
		viewCreator[i]->CreateDSVWrapper(_dev);

		// 行列用cbv作成
		viewCreator[i]->CreateCBV4Matrix(_dev);
		// pmdモデルのマテリアル、テクスチャ、sph用ビューを作成。これがないとモデル真っ黒になる。
		viewCreator[i]->CreateCBVSRV4MateriallTextureSph(_dev);

		// ガウシアンぼかし用ウェイト、バッファー作成、マッピング、ディスクリプタヒープ作成、ビュー作成まで
		auto weights = Utility::GetGaussianWeight(8, 5.0f);
		bufferHeapCreator[i]->CreateConstBufferOfGaussian(_dev, weights);
		mappingExecuter[i]->MappingGaussianWeight(weights);
		//bufferHeapCreator->CreateEffectHeap(_dev);
		//viewCreator->CreateCBV4GaussianView(_dev);

		// マルチパス用ビュー作成
		peraPolygon->CreatePeraView(_dev);
		viewCreator[i]->CreateRTV4Multipasses(_dev);
		viewCreator[i]->CreateSRV4Multipasses(_dev);
	}
// 初期化処理9：フェンスの生成
	//	ID3D12Fence* _fence = nullptr;
	//	UINT64 _fenceVal = 0;
	//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

// 初期化処理10：イベントハンドルの作成
// 初期化処理11：GPUの処理完了待ち

	return true;
}

void AppD3DX12::Run() {
	MSG msg = {};
	auto cbv_srv_Size = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < strModelNum; ++i)
	{
		pmdActor[i]->PlayAnimation(); // アニメーション開始時刻の取得
		pmdActor[i]->MotionUpdate(_duration);
	}

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリ終了時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
		{
			printf("%s", "quit");
			break;
		}

		for (int i = 0; i < strModelNum; ++i)
		{
			auto dsvh = bufferHeapCreator[i]->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();


			DrawLightMap(i, cbv_srv_Size); // draw lightmap

			DrawPeraPolygon(i); // draw background polygon

			//DrawModel(i); // draw pmd model

			//DrawShrinkTextureForBlur(i); // draw shrink buffer
		}

		DrawModel(0, cbv_srv_Size); // draw pmd model

		DrawShrinkTextureForBlur(0, cbv_srv_Size); // draw shrink buffer
				
		DrawAmbientOcclusion(0, cbv_srv_Size); // draw AO
		
		DrawBackBuffer(cbv_srv_Size); // draw back buffer

		//コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
		_cmdList->Close();

		//コマンドキューの実行
		ID3D12CommandList* cmdLists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//ID3D12FenceのSignalはCPU側のフェンスで即時実行
		//ID3D12CommandQueueのSignalはGPU側のフェンスで
		//コマンドキューに対する他のすべての操作が完了した後にフェンス更新
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

		while (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			//イベント発生待ち
			WaitForSingleObject(event, INFINITE);
			//イベントハンドルを閉じる
			CloseHandle(event);
		}

		_cmdAllocator->Reset();//コマンド アロケーターに関連付けられているメモリを再利用する
		_cmdList->Reset(_cmdAllocator.Get(), nullptr);//コマンドリストを、新しいコマンドリストが作成されたかのように初期状態にリセット

		for (int i = 0; i < strModelNum; ++i)
		{
			//行列情報の更新
			//pmdMaterialInfo->angle += 0.01f;
			//pmdMaterialInfo->angle = 200.0f;
			//pmdMaterialInfo[i]->worldMat = XMMatrixRotationY(pmdMaterialInfo[i]->angle);
			pmdMaterialInfo[0]->worldMat = XMMatrixTranslation(0.0f, 0, -5.0f);
			pmdMaterialInfo[1]->worldMat = XMMatrixTranslation(-24.0f, 0, 20.0f);
			pmdMaterialInfo[2]->worldMat = XMMatrixTranslation(15.0f, 0, 10.0f);
			pmdMaterialInfo[i]->mapMatrix->world = pmdMaterialInfo[i]->worldMat;
			pmdMaterialInfo[i]->mapMatrix4Lightmap->world = pmdMaterialInfo[i]->worldMat; // for AO!!!

			// モーション用行列の更新と書き込み
			pmdActor[i]->MotionUpdate(pmdActor[i]->GetDuration());
			pmdActor[i]->UpdateVMDMotion();
			//pmdActor->RecursiveMatrixMultiply(XMMatrixIdentity());
			//pmdActor->IKSolve();
			std::copy(boneMatrices[i]->begin(), boneMatrices[i]->end(), pmdMaterialInfo[i]->mapMatrix->bones);
			std::copy(boneMatrices[i]->begin(), boneMatrices[i]->end(), pmdMaterialInfo[i]->mapMatrix4Lightmap->bones); // for AO!!!
		}
		//フリップしてレンダリングされたイメージをユーザーに表示
		_swapChain->Present(1, 0);		
	}

	delete bufferGPLSetting;
	delete bufferShaderCompile;
	
	delete lightMapGPLSetting;	
	delete lightMapShaderCompile;
	
	for (int i = 0; i < strModelNum; ++i)
	{
		delete viewCreator[i];
		delete mappingExecuter[i];
		delete bufferHeapCreator[i];
		delete textureTransporter[i];
		delete pmdActor[i];
		delete vmdMotionInfo[i];
		delete pmdMaterialInfo[i];
	}
	UnregisterClass(prepareRenderingWindow->GetWNDCCLASSEX().lpszClassName, prepareRenderingWindow->GetWNDCCLASSEX().hInstance);


	delete textureLoader;


	delete settingShaderCompile;
	delete gPLSetting;

	
	delete prepareRenderingWindow;
	delete aoShaderCompile;
	delete aoGPLSetting;

	delete peraGPLSetting;
	delete peraLayout;
	delete peraPolygon;
	delete peraShaderCompile;	

	//delete bufferSetRootSignature;
	//delete lightMapRootSignature;
	//delete setRootSignature;
	//delete peraSetRootSignature;
}

void AppD3DX12::DrawLightMap(unsigned int modelNum, UINT buffSize)
{
	constexpr uint32_t shadow_difinition = 1024;
	D3D12_VIEWPORT vp = CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);
	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);

	auto dsvh = bufferHeapCreator[modelNum]->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

	_cmdList->SetPipelineState(lightMapGPLSetting->GetPipelineState().Get());
	_cmdList->SetGraphicsRootSignature(lightMapRootSignature->GetRootSignature().Get());

	dsvh.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvh);
	_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア
	//画面クリア
	//_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, viewCreator[modelNum]->GetVbView());

	_cmdList->IASetIndexBuffer(viewCreator[modelNum]->GetIbView());

	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[modelNum]->GetCBVSRVHeap().GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable
	(
		0, // バインドのスロット番号
		bufferHeapCreator[modelNum]->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
	);

	auto materialHandle2 = bufferHeapCreator[modelNum]->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	auto inc2 = buffSize;
	auto materialHInc2 = inc2 * 5; // 行列cbv + (material cbv+テクスチャsrv+sph srv+spa srv+toon srv)
	materialHandle2.ptr += inc2; // この処理の直前に行列用CBVをｺﾏﾝﾄﾞﾘｽﾄにセットしたため
	unsigned int idxOffset2 = 0;

	for (auto m : pmdMaterialInfo[modelNum]->materials)
	{
		_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle2);
		//インデックス付きインスタンス化されたプリミティブを描画
		_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset2, 0, 0); // instanceid 0:通常、1:影

		materialHandle2.ptr += materialHInc2;
		idxOffset2 += m.indiceNum;
	}

	//_cmdList->DrawIndexedInstanced(pmdMaterialInfo[modelNum]->vertNum, 1, 0, 0, 0);

	// ライトマップ状態をﾚﾝﾀﾞﾘﾝｸﾞﾀｰｹﾞｯﾄに変更する
	D3D12_RESOURCE_BARRIER barrierDesc4LightMap = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetLightMapBuff().Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4LightMap);
}

void AppD3DX12::DrawPeraPolygon(unsigned int modelNum)
{
	//// マルチパス1パス目

	D3D12_RESOURCE_BARRIER barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetMultipassBuff().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Multi);

	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

	auto rtvHeapPointer = bufferHeapCreator[modelNum]->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	_cmdList->SetGraphicsRootSignature(peraSetRootSignature->GetRootSignature().Get());
	// no need SetDescriptorHeaps, SetGraphicsRootDescriptorTable, because it only needs rendering.

	_cmdList->SetPipelineState(peraGPLSetting->GetPipelineState().Get());
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// ﾏﾙﾁﾊﾟｽﾘｿｰｽﾊﾞﾘｱ元に戻す
	barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetMultipassBuff().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Multi);
}

void AppD3DX12::DrawModel(unsigned int modelNum, UINT buffSize)
{
	//リソースバリアの準備。ｽﾜｯﾌﾟﾁｪｰﾝﾊﾞｯｸﾊﾞｯﾌｧは..._COMMONを初期状態とする決まり。これはcolor
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = bufferHeapCreator[modelNum]->GetMultipassBuff2().Get();
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//リソースバリア：リソースへの複数のアクセスを同期する必要があることをドライバーに通知
	_cmdList->ResourceBarrier(1, &BarrierDesc);


	// normal
	D3D12_RESOURCE_BARRIER barrierDesc4test = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetMultipassBuff3().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4test);

	// bloom
	D3D12_RESOURCE_BARRIER barrierDesc4Bloom = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetBloomBuff()[0].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Bloom);


	// モデル描画
	_cmdList->SetPipelineState(gPLSetting->GetPipelineState().Get());
	_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature().Get());
	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

	auto dsvh = bufferHeapCreator[modelNum]->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[3];
	auto baseH = bufferHeapCreator[modelNum]->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	uint32_t offset = 1; // start from No.2 RTV
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, incSize * offset);
		offset += 1;
	}
	_cmdList->OMSetRenderTargets(3, handles, false, &dsvh);

	// レンダーターゲットと深度ステンシル(両方シェーダーが認識出来ないビュー)はCPU記述子ハンドルを設定してパイプラインに直バインド
	// なのでこの二種類のビューはマッピングしなかった
	//_cmdList->OMSetRenderTargets(2, rtvs/*&handle*/, false, &dsvh);
	_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア

	//画面クリア
	float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
	_cmdList->ClearRenderTargetView(handles[0], clearColor, 0, nullptr);
	_cmdList->ClearRenderTargetView(handles[1], clearColor, 0, nullptr);
	clearColor[0] = 0;
	clearColor[1] = 0;
	clearColor[2] = 0;
	_cmdList->ClearRenderTargetView(handles[2], clearColor, 0, nullptr);

	//プリミティブ型に関する情報と、入力アセンブラーステージの入力データを記述するデータ順序をバインド
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 描画されている複数のモデルを描画していく
	for (int i = 0; i < strModelNum; ++i)
	{
		//頂点バッファーのCPU記述子ハンドルを設定
		_cmdList->IASetVertexBuffers(0, 1, viewCreator[i]->GetVbView());

		//インデックスバッファーのビューを設定
		_cmdList->IASetIndexBuffer(viewCreator[i]->GetIbView());

		//ディスクリプタヒープ設定および
		//ディスクリプタヒープとルートパラメータの関連付け
		//ここでルートシグネチャのテーブルとディスクリプタが関連付く
		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[i]->GetCBVSRVHeap().GetAddressOf());
		_cmdList->SetGraphicsRootDescriptorTable
		(
			0, // バインドのスロット番号
			bufferHeapCreator[i]->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
		);

		//////テキストのように同時に二つの同タイプDHをセットすると、グラボによっては挙動が変化する。
		////// 二つ目のセットによりNS300/Hではモデルが表示されなくなった。
		//////_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		//////_cmdList->SetGraphicsRootDescriptorTable
		//////(
		//////	1, // バインドのスロット番号
		//////	bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
		//////);

		// マテリアルのディスクリプタヒープをルートシグネチャのテーブルにバインドしていく
		// CBV:1つ(matrix)、SRV:4つ(colortex, graytex, spa, sph)が対象。SetRootSignature.cpp参照。
		auto materialHandle = bufferHeapCreator[i]->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		auto inc = buffSize;
		auto materialHInc = inc * 5; // 行列cbv + (material cbv+テクスチャsrv+sph srv+spa srv+toon srv)
		materialHandle.ptr += inc; // この処理の直前に行列用CBVをｺﾏﾝﾄﾞﾘｽﾄにセットしたため
		unsigned int idxOffset = 0;

		// (たぶん)DrawIndexedInstancedによる描画の前にSRVからのテクスチャ取得を終えていないとデータがシェーダーに通らない
		// なお、このパスでのデプスも描画と同時に渡しているが参照出来ないのは、リソース状態がdepth_writeのままだからと思われる
		_cmdList->SetGraphicsRootDescriptorTable(2, materialHandle); // デプスマップ格納
		materialHandle.ptr += inc;
		_cmdList->SetGraphicsRootDescriptorTable(3, materialHandle); // ライトマップ格納
		materialHandle.ptr += inc;

		for (auto m : pmdMaterialInfo[i]->materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
			//インデックス付きインスタンス化されたプリミティブを描画
			_cmdList->DrawIndexedInstanced(m.indiceNum, 2, idxOffset, 0, 0); // instanceid 0:通常、1:影

			materialHandle.ptr += materialHInc;
			idxOffset += m.indiceNum;
		}

	}
	// color
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// normal
	barrierDesc4test = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetMultipassBuff3().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4test);

	// bloom
	barrierDesc4Bloom = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetBloomBuff()[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Bloom);
}


void AppD3DX12::DrawShrinkTextureForBlur(unsigned int modelNum, UINT buffSize)
{
	_cmdList->SetPipelineState(bloomGPLSetting->GetPipelineState().Get());
	_cmdList->SetGraphicsRootSignature(bloomRootSignature->GetRootSignature().Get());

	// set vertex buffer
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());

	// No need to set luminance buffer to shader resource, cause other method implement it.

	// high luminance blur renderer status to
	D3D12_RESOURCE_BARRIER barrierDesc4Shrink = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetBloomBuff()[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Shrink);

	// model blur renderer status to
	D3D12_RESOURCE_BARRIER barrierDesc4ShrinkModel = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetBloomBuff()[2].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4ShrinkModel);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[2];
	auto baseH = bufferHeapCreator[modelNum]->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	uint32_t offset = 4;
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, incSize * offset);
		offset += 1;
	}	

	_cmdList->OMSetRenderTargets(2, handles, false, nullptr);
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	_cmdList->ClearRenderTargetView(handles[0], clearColor, 0, nullptr);
	_cmdList->ClearRenderTargetView(handles[1], clearColor, 0, nullptr);

	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[modelNum]->GetMultipassSRVHeap().GetAddressOf());

	// bloom texture
	auto srvHandle = bufferHeapCreator[modelNum]->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	srvHandle.ptr += buffSize; // model texture
	_cmdList->SetGraphicsRootDescriptorTable(1, srvHandle);
	srvHandle.ptr += buffSize; // gaussian value
	_cmdList->SetGraphicsRootDescriptorTable(2, srvHandle); // table[2] is for gaussian value
	srvHandle.ptr += buffSize * 6; // bloom texture
	_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);

	auto desc = bufferHeapCreator[modelNum]->GetBloomBuff()[0]->GetDesc();
	D3D12_VIEWPORT vp = {};
	D3D12_RECT sr = {};

	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.Height = desc.Height / 2;
	vp.Width = desc.Width / 2;
	sr.top = 0;
	sr.left = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	for (int i = 0; i < 8; i++)
	{
		_cmdList->RSSetViewports(1, &vp);
		_cmdList->RSSetScissorRects(1, &sr);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		// draw and shift down to draw next texture
		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;

		// halve width and height
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	// change resource from render target to shader resource
	barrierDesc4Shrink.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc4Shrink.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &barrierDesc4Shrink);

	// change resource from render target to shader resource
	barrierDesc4ShrinkModel.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc4ShrinkModel.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &barrierDesc4ShrinkModel);
}

void AppD3DX12::DrawAmbientOcclusion(unsigned int modelNum, UINT buffSize)
{
	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

	for (int i = 0; i < strModelNum; ++i)
	{
		// デプスマップ用バッファの状態を読み込み可能に変える
		D3D12_RESOURCE_BARRIER barrierDesc4DepthMap = CD3DX12_RESOURCE_BARRIER::Transition
		(
			bufferHeapCreator[i]->/*GetDepthMapBuff*/GetDepthBuff().Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	}

	// AO renderer status to
	D3D12_RESOURCE_BARRIER barrierDesc4AO = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator[modelNum]->GetAOBuff().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4AO);
		
	auto baseH = bufferHeapCreator[modelNum]->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 6;
	baseH.ptr += incSize;

	auto dsvh = bufferHeapCreator[modelNum]->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
	dsvh.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV) * 2;
	_cmdList->OMSetRenderTargets(1, &baseH, false, &dsvh);
	_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア

	float clearColor[] = { 0.5f, 0.0f, 1.0f, 1.0f };
	_cmdList->ClearRenderTargetView(baseH, clearColor, 0, nullptr);

	_cmdList->SetGraphicsRootSignature(aoRootSignature->GetRootSignature().Get());


	_cmdList->SetPipelineState(aoGPLSetting->GetPipelineState().Get());
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);// 描画されている複数のモデルを描画していく

	
	for (int i = 0; i < strModelNum; ++i)
	{
		// ﾃﾞﾌﾟｽﾏｯﾌﾟと法線マップはbufferHeapCreator[0]のものを利用する。シーン行列はそれぞれのモデルのものを利用する。
		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[0]->GetMultipassSRVHeap().GetAddressOf());
		auto srvHandle00 = bufferHeapCreator[0]->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		srvHandle00.ptr += buffSize * 4; // depthmap
		_cmdList->SetGraphicsRootDescriptorTable(2, srvHandle00);
		srvHandle00.ptr += buffSize * 3; // normalmap
		_cmdList->SetGraphicsRootDescriptorTable(3, srvHandle00);

		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[i]->GetMultipassSRVHeap().GetAddressOf());
		auto srvHandle = bufferHeapCreator[i]->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		srvHandle.ptr += buffSize * 6; // scene matrix

		_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
		//頂点バッファーのCPU記述子ハンドルを設定
		_cmdList->IASetVertexBuffers(0, 1, viewCreator[/*modelNum*/i]->GetVbView());

		//インデックスバッファーのビューを設定
		_cmdList->IASetIndexBuffer(viewCreator[/*modelNum*/i]->GetIbView());


		//_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[/*modelNum*/i]->GetMultipassSRVHeap().GetAddressOf());

		//auto srvHandle = bufferHeapCreator[/*modelNum*/i]->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		//srvHandle.ptr += buffSize * 4; // depthmap
		//_cmdList->SetGraphicsRootDescriptorTable(2, srvHandle);
		//srvHandle.ptr += buffSize * 2; // scene matrix
		//_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);
		//srvHandle.ptr += buffSize; // normalmap
		//_cmdList->SetGraphicsRootDescriptorTable(3, srvHandle);

		unsigned int idxOffset = 0;
		//インデックス付きインスタンス化されたプリミティブを描画
		for (auto m : pmdMaterialInfo[/*modelNum*/i]->materials)
		{
			//インデックス付きインスタンス化されたプリミティブを描画
			_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset, 0, 0);
			idxOffset += m.indiceNum;
		}
	}

	//  AO renderer status to
	barrierDesc4AO.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc4AO.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &barrierDesc4AO);
}

void AppD3DX12::DrawBackBuffer(UINT buffSize)
{

	auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//現在のバックバッファをインデックスにて取得

	//for (int i = 0; i < strModelNum; ++i)
	//{
	//	// デプスマップ用バッファの状態を読み込み可能に変える
	//	D3D12_RESOURCE_BARRIER barrierDesc4DepthMap = CD3DX12_RESOURCE_BARRIER::Transition
	//	(
	//		bufferHeapCreator[i]->/*GetDepthMapBuff*/GetDepthBuff().Get(),
	//		D3D12_RESOURCE_STATE_DEPTH_WRITE,
	//		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	//	);
	//}
	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer()); // 実は重要
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer()); // 実は重要

	// ﾊﾞｯｸﾊﾞｯﾌｧに描画する
	// ﾊﾞｯｸﾊﾞｯﾌｧ状態をﾚﾝﾀﾞﾘﾝｸﾞﾀｰｹﾞｯﾄに変更する
	D3D12_RESOURCE_BARRIER barrierDesc4BackBuffer = CD3DX12_RESOURCE_BARRIER::Transition
	(
		_backBuffers[bbIdx].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);

	// only bufferHeapCreator[0]->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart() is initialized as backbuffer
	auto rtvHeapPointer = bufferHeapCreator[0]->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);
	//_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア

	float clsClr[4] = { 0.2,0.5,0.5,1.0 };
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clsClr, 0, nullptr);
	


	// 作成したﾃｸｽﾁｬの利用処理
	_cmdList->SetGraphicsRootSignature(/*peraSetRootSignature*/bufferSetRootSignature->GetRootSignature().Get());
	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator[0]->/*GetCBVSRVHeap()*/GetMultipassSRVHeap().GetAddressOf());

	auto gHandle = bufferHeapCreator[0]->/*GetCBVSRVHeap()*/GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0, gHandle);
	_cmdList->SetPipelineState(/*peraGPLSetting*/bufferGPLSetting->GetPipelineState().Get());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());

	auto gHandle2 = bufferHeapCreator[0]->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(1, gHandle2);

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(2, gHandle2); // ぼかし定数

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(3, gHandle2); // 法線マップ

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(4, gHandle2); // 深度マップ

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(5, gHandle2); // ライトマップ

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(6, gHandle2); // ライトマップ用シーン行列

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(7, gHandle2); // normalmap

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(8, gHandle2); // bloom

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(9, gHandle2); // shrinked bloom

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(10, gHandle2); // shrinked model

	gHandle2.ptr += buffSize;
	_cmdList->SetGraphicsRootDescriptorTable(11, gHandle2); // AO

	_cmdList->DrawInstanced(4, 1, 0, 0);

	// ﾊﾞｯｸﾊﾞｯﾌｧ状態をﾚﾝﾀﾞﾘﾝｸﾞﾀｰｹﾞｯﾄから元に戻す
	barrierDesc4BackBuffer.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc4BackBuffer.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);

	for (int i = 0; i < strModelNum; ++i)
	{
		// デプスマップ用バッファの状態を書き込み可能に戻す
		auto barrierDesc4DepthMap = CD3DX12_RESOURCE_BARRIER::Transition
		(
			bufferHeapCreator[i]->/*GetDepthMapBuff*/GetDepthBuff().Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);

		// ライトマップ用バッファの状態を書き込み可能に戻す
		auto barrierDesc4LightMap = CD3DX12_RESOURCE_BARRIER::Transition
		(
			bufferHeapCreator[i]->GetLightMapBuff().Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE

		);

		_cmdList->ResourceBarrier(1, &barrierDesc4LightMap);
	}
}