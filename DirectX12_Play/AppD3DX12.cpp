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

#ifdef _DEBUG
bool AppD3DX12::PrepareRendering() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	// SetRootSignatureBaseクラスのインスタンス化
	setRootSignature = new SetRootSignature;

	// SettingShaderCompileクラスのインスタンス化
	settingShaderCompile = new SettingShaderCompile;

	// VertexInputLayoutクラスのインスタンス化
	vertexInputLayout = new VertexInputLayout;
	
	// PMDファイルの読み込み
	pmdMaterialInfo = new PMDMaterialInfo;
	if (FAILED(pmdMaterialInfo->ReadPMDHeaderFile(strModelPath))) return false;

	// VMDモーションファイルの読み込み
	vmdMotionInfo = new VMDMotionInfo;
	if (FAILED(vmdMotionInfo->ReadVMDHeaderFile(strMotionPath))) return false;

	// PMDActorクラスのインスタンス化
	pmdActor = new PMDActor(pmdMaterialInfo, vmdMotionInfo);

	// GraphicsPipelineSettingクラスのインスタンス化
	gPLSetting = new GraphicsPipelineSetting(vertexInputLayout);

	// アニメーション用の回転・並行移動行列の参照準備
	boneMatrices = new std::vector<DirectX::XMMATRIX>;
	boneMatrices = pmdActor->GetMatrices();
	bNodeTable = pmdMaterialInfo->GetBoneNode();

	// レンダリングウィンドウ設定
	prepareRenderingWindow = new PrepareRenderingWindow;
	prepareRenderingWindow->CreateAppWindow();

	// TextureLoaderクラスのインスタンス化
	textureLoader = new TextureLoader;

	// BufferHeapCreatorクラスのインスタンス化
	bufferHeapCreator = new BufferHeapCreator(pmdMaterialInfo, prepareRenderingWindow, textureLoader);

	// TextureTransporterクラスのインスタンス化
	textureTransporter = new TextureTransporter(pmdMaterialInfo, bufferHeapCreator);

	// MappingExecuterクラスのインスタンス化
	mappingExecuter = new MappingExecuter(pmdMaterialInfo, bufferHeapCreator);

	// ViewCreatorクラスのインスタンス化
	viewCreator = new ViewCreator(pmdMaterialInfo, bufferHeapCreator);

	// レンダリングウィンドウ表示
	ShowWindow(prepareRenderingWindow->GetHWND(), SW_SHOW);

	// ビューポートとシザー領域の設定
	prepareRenderingWindow->SetViewportAndRect();

	// ﾏﾙﾁﾊﾟｽ関連ｸﾗｽ群
	peraLayout = new PeraLayout;
	peraGPLSetting = new PeraGraphicsPipelineSetting(peraLayout); //TODO PeraLayout,VertexInputLayoutｸﾗｽの基底クラスを作ってそれに対応させる
	peraPolygon = new PeraPolygon;
	peraSetRootSignature = new PeraSetRootSignature;
	peraShaderCompile = new PeraShaderCompile;
	bufferGPLSetting = new PeraGraphicsPipelineSetting(peraLayout);
	bufferSetRootSignature = new PeraSetRootSignature;
	bufferShaderCompile = new BufferShaderCompile;
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

	result = infoQueue->PushStorageFilter(&filter);
	// ついでにエラーメッセージでブレークさせる
	result = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

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
	
	//RTV 記述子ヒープ領域の確保
	bufferHeapCreator->SetRTVHeapDesc();
	//RTV用記述子ヒープの生成　ID3D12DescriptorHeap：記述子の連続したコレクション
	result = bufferHeapCreator->CreateRTVHeap(_dev);

	//以下のように記述することでスワップチェーンの持つ情報を新たなDescオブジェクトにコピーできる
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//スワップチェーンの説明
	//result = _swapChain->GetDesc(&swcDesc);//SWCの説明を取得する

//初期化処理６：フレームリソース(各フレームのレンダーターゲットビュー)を作成
	_backBuffers.resize(swapChainDesc.BufferCount); // ｽﾜｯﾌﾟﾁｪｰﾝﾊﾞｯｸﾊﾞｯﾌｧｰのﾘｻｲｽﾞ
	handle = bufferHeapCreator->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();//ヒープの先頭を表す CPU 記述子ハンドルを取得

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

// 初期化処理3：頂点入力レイアウトの作成及び
// 初期化処理4：パイプライン状態オブジェクト(PSO)のDesc記述してオブジェクト作成
	result = gPLSetting->CreateGPStateWrapper(_dev, setRootSignature, _vsBlob, _psBlob);

	// ﾏﾙﾁﾊﾟｽ用
	result = peraGPLSetting->CreateGPStateWrapper(_dev, peraSetRootSignature, _vsMBlob, _psMBlob);

	// 表示用
	result = bufferGPLSetting->CreateGPStateWrapper(_dev, bufferSetRootSignature, _vsBackbufferBlob, _psBackbufferBlob);

// 初期化処理5：コマンドリスト生成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

// 初期化処理6：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
	//cmdList->Close();

// 初期化処理7：各バッファーを作成して頂点情報を読み込み

	//頂点バッファーの作成(リソースと暗黙的なヒープの作成) 
	result = bufferHeapCreator->CreateBufferOfVertex(_dev);

	//インデックスバッファーを作成(リソースと暗黙的なヒープの作成)
	result = bufferHeapCreator->CreateBufferOfIndex(_dev);

	//デプスバッファーを作成
	result = bufferHeapCreator->CreateBufferOfDepth(_dev);

	//ファイル形式毎のテクスチャロード処理
	textureLoader->LoadTexture();
	
	// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	metaData.resize(pmdMaterialInfo->materialNum);
	img.resize(pmdMaterialInfo->materialNum);
	ScratchImage scratchImg = {};
	bufferHeapCreator->CreateUploadAndReadBuff(_dev, strModelPath, metaData, img); // バッファ作成
	
	// トゥーンテクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	toonMetaData.resize(pmdMaterialInfo->materialNum);
	toonImg.resize(pmdMaterialInfo->materialNum);
	bufferHeapCreator->CreateToonUploadAndReadBuff(_dev, strModelPath, toonMetaData, toonImg); // バッファ作成

	//行列用定数バッファーの生成
	pmdMaterialInfo->worldMat = XMMatrixIdentity();
	//auto worldMat = XMMatrixRotationY(15.0f);
	pmdMaterialInfo->angle = 0.0f;

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
	result = bufferHeapCreator->CreateConstBufferOfWVPMatrix(_dev);
	
	//マテリアル用定数バッファーの生成
	result = bufferHeapCreator->CreateConstBufferOfMaterial(_dev);

	// マルチパスレンダリング用に書き込み先リソースの作成
    // 作成済みのヒープ情報を使ってもう一枚レンダリング先を用意
	// 使っているバックバッファーの情報を利用する
	auto& bbuff = _backBuffers[0];
	auto mutipassResDesc = bbuff->GetDesc();
	// RTV,SRV用バッファーと各ヒープ作成
	result = bufferHeapCreator->CreateRenderBufferForMultipass(_dev, mutipassResDesc);
	bufferHeapCreator->CreateMultipassRTVHeap(_dev);
	bufferHeapCreator->CreateMultipassSRVHeap(_dev);

	//頂点バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的に頂点データをコピーする。
	mappingExecuter->MappingVertBuff();

	//インデクスバッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	mappingExecuter->MappingIndexOfVertexBuff();

	//行列用定数バッファーのマッピング
	result = bufferHeapCreator->GetMatrixBuff()->Map(0, nullptr, (void**)&pmdMaterialInfo->mapMatrix);
	pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;
	pmdMaterialInfo->mapMatrix->view = viewMat;
	pmdMaterialInfo->mapMatrix->proj = projMat;
	pmdMaterialInfo->mapMatrix->eye = eye;

	//マテリアル用バッファーへのマッピング
	mappingExecuter->MappingMaterialBuff();
	
	// テクスチャのアップロード用バッファへのマッピング
	mappingExecuter->TransferTexUploadToBuff(img);
	// テクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	textureTransporter->TransportTexture(_cmdList, _cmdAllocator, _cmdQueue, metaData, img, _fence, _fenceVal);

	// トゥーンテクスチャも同様にマッピング
	mappingExecuter->TransferToonTexUploadToBuff(toonImg);
	// トゥーンテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
	textureTransporter->TransportToonTexture(_cmdList, _cmdAllocator, _cmdQueue, toonMetaData, toonImg, _fence, _fenceVal);

	//CBV,SRVディスクリプタヒープ作成(行列、テクスチャに利用)
	result = bufferHeapCreator->CreateCBVSRVHeap(_dev);

	//DSVビュー用にディスクリプタヒープ作成
	result = bufferHeapCreator->CreateDSVHeap(_dev);

// 初期化処理8：各ビューを作成

	// Vertexビュー作成
	viewCreator->CreateVertexBufferView();

	// Indexビュー作成
	viewCreator->CreateIndexBufferView();

	// DSV作成
	viewCreator->CreateDSVWrapper(_dev);

	// 行列用cbv作成
	viewCreator->CreateCBV4Matrix(_dev);
	// pmdモデルのマテリアル、テクスチャ、sph用ビューを作成。これがないとモデル真っ黒になる。
	viewCreator->CreateCBVSRV4MateriallTextureSph(_dev);

	// ガウシアンぼかし用ウェイト、バッファー作成、マッピング、ディスクリプタヒープ作成、ビュー作成まで
	auto weights = Utility::GetGaussianWeight(8, 5.0f);
	bufferHeapCreator->CreateConstBufferOfGaussian(_dev, weights);
	mappingExecuter->MappingGaussianWeight(weights);
	//bufferHeapCreator->CreateEffectHeap(_dev);
	//viewCreator->CreateCBV4GaussianView(_dev);

	// マルチパス用ビュー作成
	peraPolygon->CreatePeraView(_dev);
	viewCreator->CreateRTV4Multipasses(_dev);
	viewCreator->CreateSRV4Multipasses(_dev);

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
	pmdActor->PlayAnimation(); // アニメーション開始時刻の取得
	pmdActor->MotionUpdate(_duration);
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリ終了時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}



		auto dsvh = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

		//// マルチパス1パス目

		D3D12_RESOURCE_BARRIER barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
		(
			bufferHeapCreator->GetMultipassBuff().Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &barrierDesc4Multi);

		_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer()); // 実は重要
		_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer()); // 実は重要
		
		auto rtvHeapPointer = bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
		//auto dsvHeapHandle = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
		//rtvHeapPointer.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);
		_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
		//_cmdList->ClearDepthStencilView(dsvHeapHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア
		_cmdList->SetGraphicsRootSignature(peraSetRootSignature->GetRootSignature().Get());

		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->GetMultipassSRVHeap().GetAddressOf());
		auto mHandle = bufferHeapCreator->GetMultipassSRVHeap().Get()->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(0, mHandle);

		_cmdList->SetPipelineState(peraGPLSetting->GetPipelineState().Get());
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());
		_cmdList->DrawInstanced(4, 1, 0, 0);


		// ﾏﾙﾁﾊﾟｽﾘｿｰｽﾊﾞﾘｱ元に戻す
		barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
		(
			bufferHeapCreator->GetMultipassBuff().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		_cmdList->ResourceBarrier(1, &barrierDesc4Multi);





		// ﾏﾙﾁﾊﾟｽ2パス目
		//リソースバリアの準備。ｽﾜｯﾌﾟﾁｪｰﾝﾊﾞｯｸﾊﾞｯﾌｧは..._COMMONを初期状態とする決まり。
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = /*_backBuffers[bbIdx]*/bufferHeapCreator->GetMultipassBuff2().Get();
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = /*D3D12_RESOURCE_STATE_PRESENT*/D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//リソースバリア：リソースへの複数のアクセスを同期する必要があることをドライバーに通知
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		// モデル描画
		_cmdList->SetPipelineState(gPLSetting->GetPipelineState().Get());
		_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature().Get());
		_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
		_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

		//ハンドルの初期値アドレスにバッファインデックスを乗算し、各ハンドルの先頭アドレスを計算
		handle = bufferHeapCreator->/*GetRTVHeap()*/GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart(); // auto rtvhでhandleに上書きでも可
		//handle.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		handle.ptr += /*(bbIdx + 1) * */_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//auto dsvh = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
		// レンダーターゲットと深度ステンシル(両方シェーダーが認識出来ないビュー)はCPU記述子ハンドルを設定してパイプラインに直バインド
		// なのでこの二種類のビューはマッピングしなかった
		// ★戻す
		_cmdList->OMSetRenderTargets(1, &handle, false, &dsvh);
		_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア

		//画面クリア
		//float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);

		//プリミティブ型に関する情報と、入力アセンブラーステージの入力データを記述するデータ順序をバインド
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//頂点バッファーのCPU記述子ハンドルを設定
		_cmdList->IASetVertexBuffers(0, 1, viewCreator->GetVbView());

		//インデックスバッファーのビューを設定
		_cmdList->IASetIndexBuffer(viewCreator->GetIbView());

		//ディスクリプタヒープ設定および
		//ディスクリプタヒープとルートパラメータの関連付け
		//ここでルートシグネチャのテーブルとディスクリプタが関連付く
		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->GetCBVSRVHeap().GetAddressOf());
		_cmdList->SetGraphicsRootDescriptorTable
		(
			0, // バインドのスロット番号
			bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
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
		auto materialHandle = bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto materialHInc = inc * 5; // 行列cbv + (material cbv+テクスチャsrv+sph srv+spa srv+toon srv)
		materialHandle.ptr += inc; // この処理の直前に行列用CBVをｺﾏﾝﾄﾞﾘｽﾄにセットしたため
		unsigned int idxOffset = 0;

		for (auto m : pmdMaterialInfo->materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
			//インデックス付きインスタンス化されたプリミティブを描画
			_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset, 0, 0);

			materialHandle.ptr += materialHInc;
			idxOffset += m.indiceNum;
		}

		//_cmdList->DrawInstanced(vertNum ,1, 0, 0);

		//ﾊﾞｯｸﾊﾞｯﾌｧ表示前にリソースをCOMMON状態に移行
		//コマンドリストクローズ後は、コマンドリストが特定の呼び出し(Reset())以外は受け付けず、以下3行はエラーになる
		//クローズ後にコマンドキューを実行しているが、ここでリソースの状態が適用される。ここまでにCOMMONから状態を
		//変更させておく必要があるが、実質は●●●から～クローズまでに変更させる必要がある。
		//★戻す
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = /*D3D12_RESOURCE_STATE_PRESENT*/D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		_cmdList->ResourceBarrier(1, &BarrierDesc);





		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//現在のバックバッファをインデックスにて取得

		// ﾊﾞｯｸﾊﾞｯﾌｧに描画する
		// ﾊﾞｯｸﾊﾞｯﾌｧ状態をﾚﾝﾀﾞﾘﾝｸﾞﾀｰｹﾞｯﾄに変更する
		D3D12_RESOURCE_BARRIER barrierDesc4BackBuffer = CD3DX12_RESOURCE_BARRIER::Transition
		(
			_backBuffers[bbIdx].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);

		rtvHeapPointer = bufferHeapCreator->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
		rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);

		float clsClr[4] = { 0.2,0.5,0.5,1.0 };
		_cmdList->ClearRenderTargetView(rtvHeapPointer, clsClr, 0, nullptr);

		// 作成したﾃｸｽﾁｬの利用処理
		_cmdList->SetGraphicsRootSignature(/*peraSetRootSignature*/bufferSetRootSignature->GetRootSignature().Get());
		_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->/*GetCBVSRVHeap()*/GetMultipassSRVHeap().GetAddressOf());

		auto gHandle = bufferHeapCreator->/*GetCBVSRVHeap()*/GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		_cmdList->SetGraphicsRootDescriptorTable(0, gHandle);
		_cmdList->SetPipelineState(/*peraGPLSetting*/bufferGPLSetting->GetPipelineState().Get());

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());

		auto gHandle2 = bufferHeapCreator->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
		gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cmdList->SetGraphicsRootDescriptorTable(1, gHandle2);
		
		gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		_cmdList->SetGraphicsRootDescriptorTable(2, gHandle2);

		_cmdList->DrawInstanced(4, 1, 0, 0);

		// ﾊﾞｯｸﾊﾞｯﾌｧ状態をﾚﾝﾀﾞﾘﾝｸﾞﾀｰｹﾞｯﾄから元に戻す
		barrierDesc4BackBuffer.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc4BackBuffer.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);





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

		//行列情報の更新
		//pmdMaterialInfo->angle += 0.01f;
		pmdMaterialInfo->angle = 200.0f;
		pmdMaterialInfo->worldMat = XMMatrixRotationY(pmdMaterialInfo->angle);
		pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;

		// モーション用行列の更新と書き込み
		pmdActor->MotionUpdate(pmdActor->GetDuration());
		pmdActor->UpdateVMDMotion();
		//pmdActor->RecursiveMatrixMultiply(XMMatrixIdentity());
		//pmdActor->IKSolve();
		std::copy(boneMatrices->begin(), boneMatrices->end(), pmdMaterialInfo->mapMatrix->bones);

		//フリップしてレンダリングされたイメージをユーザーに表示
		_swapChain->Present(1, 0);
	}

	delete viewCreator;
	delete mappingExecuter;
	UnregisterClass(prepareRenderingWindow->GetWNDCCLASSEX().lpszClassName, prepareRenderingWindow->GetWNDCCLASSEX().hInstance);

	delete bufferHeapCreator;
	delete textureTransporter;
	delete textureLoader;

	delete pmdActor;
	delete settingShaderCompile;
	delete setRootSignature;
	delete gPLSetting;

	delete vmdMotionInfo;
	delete prepareRenderingWindow;
	delete pmdMaterialInfo;

	delete peraGPLSetting;
	delete peraLayout;
	delete peraPolygon;
	delete peraSetRootSignature;
	delete peraShaderCompile;

	//delete bufferGPLSetting;
	//delete bufferSetRootSignature;
	//delete bufferShaderCompile;
}