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
	// PMDファイルの読み込み
	pmdMaterialInfo = new PMDMaterialInfo;
	if (FAILED(pmdMaterialInfo->ReadPMDHeaderFile(strModelPath))) return false;

	// VMDモーションファイルの読み込み
	vmdMotionInfo = new VMDMotionInfo;
	if (FAILED(vmdMotionInfo->ReadVMDHeaderFile(strMotionPath))) return false;

	// PMDActorクラスのインスタンス化
	pmdActor = new PMDActor(pmdMaterialInfo, vmdMotionInfo);
	//pmdActor->SolveCCDIK(pmdMaterialInfo->GetpPMDIKData()[0]);

	// GraphicsPipelineSettingクラスのインスタンス化
	gPLSetting = new GraphicsPipelineSetting;

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

	// レンダリングウィンドウ表示
	ShowWindow(prepareRenderingWindow->GetHWND(), SW_SHOW);

	// ビューポートとシザー領域の設定
	prepareRenderingWindow->SetViewportAndRect();
}

bool AppD3DX12::PipelineInit(){
//●パイプライン初期化　処理１～７
//初期化処理１：デバッグレイヤーをオンに
#ifdef _DEBUG
	Utility::EnableDebugLayer();
#endif
	// 各種デバイスの初期設定
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
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	//記述子ヒープの生成　ID3D12DescriptorHeap：記述子の連続したコレクション
	//ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));

	//以下のように記述することでスワップチェーンの持つ情報を新たなDescオブジェクトにコピーできる
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//スワップチェーンの説明
	//result = _swapChain->GetDesc(&swcDesc);//SWCの説明を取得する

//初期化処理６：フレームリソース(各フレームのレンダーターゲットビュー)を作成
	_backBuffers.resize(swapChainDesc.BufferCount);//リソースバッファー
	handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();//ヒープの先頭を表す CPU 記述子ハンドルを取得

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
	////●リソース初期化
	
	//// 初期化処理1：ルートシグネチャ設定
	setRootSignature = new SetRootSignature;
	if (FAILED(setRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}
	
	setRootSignature->SetRootsignatureParam(_dev);

	// 初期化処理2：シェーダーコンパイル

	result = D3DCompileFromFile
	(
		L"BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		_vsBlob.ReleaseAndGetAddressOf()
		, setRootSignature->GetErrorBlob().GetAddressOf()
	);

	result = D3DCompileFromFile
	(
		L"BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		_psBlob.ReleaseAndGetAddressOf()
		, setRootSignature->GetErrorBlob().GetAddressOf()
	);

	//エラーチェック
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見つかりません");
			//return 0;
			return false;
		}
		else
		{
			std::string errstr;
			errstr.resize(setRootSignature->GetErrorBlob()->GetBufferSize());

			std::copy_n((char*)setRootSignature->GetErrorBlob()->GetBufferPointer(),
				setRootSignature->GetErrorBlob()->GetBufferSize(),
				errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());

		}
	}

	// 初期化処理3：頂点入力レイアウトの作成

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		//座標
		{
			"POSITION",
			0, // 同じセマンティクスに対するインデックス
			DXGI_FORMAT_R32G32B32_FLOAT,
			0, // スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 // 一度に描画するインスタンス数
		},

		//法線ベクトル
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//uv
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0, // スロットインデックス
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//ボーン番号
		{
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT, // bone[0], bone[1]
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//法線ベクトル
		{
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//法線ベクトル
		{
			"EDGE_FLG",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};


	// 初期化処理4：パイプライン状態オブジェクト(PSO)のDesc記述してオブジェクト作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	for(int i=0;i<sizeof(inputLayout);++i)
	{
		gPLSetting->SetInputlayout(i, inputLayout[i]);
	}

	gpipeLine = gPLSetting->SetGPL(gpipeLine, _dev, _pipelineState, setRootSignature, _vsBlob, _psBlob);
	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// 初期化処理5：コマンドリスト生成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	// 初期化処理6：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
	//cmdList->Close();

	// 初期化処理7：各バッファーを作成して頂点情報を読み込み

	//頂点バッファーとインデックスバッファー用のヒーププロパティ設定
	D3D12_HEAP_PROPERTIES heapProps = {};
	bufferHeapCreator->SetVertexAndIndexHeapProp(&heapProps);

	//深度バッファー用ヒーププロパティ設定
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	bufferHeapCreator->SetDepthHeapProp(&depthHeapProps);

	//深度バッファー用リソースディスクリプタ
	D3D12_RESOURCE_DESC depthResDesc = {};
	bufferHeapCreator->SetDepthResourceDesc(&depthResDesc);

	//クリアバリュー(特定のリソースのクリア操作を最適化するために使用される値)
	D3D12_CLEAR_VALUE depthClearValue = {};
	bufferHeapCreator->SetClearValue(&depthClearValue);

	//ID3D12Resourceオブジェクトの内部パラメータ設定
	D3D12_RESOURCE_DESC vertresDesc = CD3DX12_RESOURCE_DESC::Buffer(pmdMaterialInfo->vertices.size());
	D3D12_RESOURCE_DESC indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size());
	
	//頂点バッファーの作成(リソースと暗黙的なヒープの作成) 
	result = bufferHeapCreator->CreateBufferOfVertex(_dev, heapProps, vertresDesc);

	//インデックスバッファーを作成(リソースと暗黙的なヒープの作成)
	result = bufferHeapCreator->CreateBufferOfIndex(_dev, heapProps, indicesDesc);

	//デプスバッファーを作成
	result = bufferHeapCreator->CreateBufferOfDepth(_dev, depthHeapProps, depthResDesc);

	//ファイル形式毎のテクスチャロード処理
	textureLoader->LoadTexture();
	
	// テクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	metaData.resize(pmdMaterialInfo->materialNum);
	img.resize(pmdMaterialInfo->materialNum);
	ScratchImage scratchImg = {};
	result = CoInitializeEx(0, COINIT_MULTITHREADED);	
	bufferHeapCreator->CreateUploadAndReadBuff(_dev, strModelPath, metaData, img); // バッファ作成

	// トゥーンテクスチャ用のCPU_Upload用、GPU_Read用バッファの作成
	std::string toonFilePath = "toon\\";
	struct _stat s = {};
	toonMetaData.resize(pmdMaterialInfo->materialNum);
	toonImg.resize(pmdMaterialInfo->materialNum);
	ScratchImage toonScratchImg = {};
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

	D3D12_HEAP_PROPERTIES constBuffProp = {};
	constBuffProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC constBuffResdesc = {};
	constBuffResdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);;
	_dev->CreateCommittedResource
	(
		&constBuffProp,
		D3D12_HEAP_FLAG_NONE,
		&constBuffResdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(matrixBuff.ReleaseAndGetAddressOf())
	);
	
	//マテリアル用定数バッファーの生成
	auto materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;
	D3D12_HEAP_PROPERTIES materialHeapProp = {};
	D3D12_RESOURCE_DESC materialBuffResDesc = {};
	materialHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	materialBuffResDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * pmdMaterialInfo->materialNum);

	_dev->CreateCommittedResource
	(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialBuffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
	);

	// マルチパスレンダリング用
    // 作成済みのヒープ情報を使ってもう一枚レンダリング先を用意
	auto heapDesc2 = rtvHeaps->GetDesc();

	// 使っているバックバッファーの情報を利用する
	auto& bbuff = _backBuffers[0];
	auto resDesc2 = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// レンダリング時のクリア値と同じ値
	float clsClr[4] = { 0.5,0.5,0.5,1.0 };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	auto result = _dev->CreateCommittedResource
	(
		&heapProp2,
		D3D12_HEAP_FLAG_NONE,
		&resDesc2,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_peraResource.ReleaseAndGetAddressOf())
	);

	//頂点バッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的に頂点データをコピーする。
	//CPUは暗黙的なヒープの情報を得られないため、Map関数によりVRAM上のバッファーにアドレスを割り当てた状態で
	//頂点などの情報をVRAMへコピーしている(次の３つはCPUとGPUどちらもアクセス可能なUPLOADタイプなヒープ故マップ可能)、
	//sという理解。Unmapはコメントアウトしても特に影響はないが...
	//vertMap = nullptr;
	result = bufferHeapCreator->GetVertBuff()->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(pmdMaterialInfo->vertices), std::end(pmdMaterialInfo->vertices), vertMap);
	bufferHeapCreator->GetVertBuff()->Unmap(0, nullptr);

	//インデクスバッファーの仮想アドレスをポインタにマップ(関連付け)して、仮想的にインデックスデータをコピーする。
	//mappedIdx = nullptr;
	result = bufferHeapCreator->GetIdxBuff()->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(pmdMaterialInfo->indices), std::end(pmdMaterialInfo->indices), mappedIdx);
	bufferHeapCreator->GetIdxBuff()->Unmap(0, nullptr);

	//boneMatrices = pmdMaterialInfo->GetBoneMatrices();
	//行列用定数バッファーのマッピング
	//mapMatrix = nullptr;
	result = matrixBuff->Map(0, nullptr, (void**)&pmdMaterialInfo->mapMatrix);
	pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;
	pmdMaterialInfo->mapMatrix->view = viewMat;
	pmdMaterialInfo->mapMatrix->proj = projMat;
	pmdMaterialInfo->mapMatrix->eye = eye;

	//マテリアル用バッファーへのマッピング
	//mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto m : pmdMaterialInfo->materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}
	materialBuff->Unmap(0, nullptr);

	// テクスチャアップロード用バッファーの仮想アドレスをポインタにマップ(関連付け)して、
	// 仮想的にインデックスデータをコピーする。
	// テクスチャのアップロード用バッファへのマッピング
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetTexUploadBuff()[matNum] == nullptr) continue;

		auto srcAddress = img[matNum]->pixels;
		auto rowPitch = Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);//////////////////////////////
		uint8_t* mapforImg = nullptr; // ピクセルデータのシステムメモリバッファーへのポインターがuint8_t(img->pixcel)
		result = bufferHeapCreator->GetTexUploadBuff()[matNum]->Map(0, nullptr, (void**)&mapforImg);

		// img:元データの初期アドレス(srcAddress)を元ピッチ分オフセットしながら、補正したピッチ個分(rowPitch)のアドレスを
		// mapforImgにその数分(rowPitch)オフセットを繰り返しつつコピーしていく
		//std::copy_n(img->pixels, img->slicePitch, mapforImg);
		for (int i = 0; i < img[matNum]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[matNum]->rowPitch;
			mapforImg += rowPitch;
		}

		bufferHeapCreator->GetTexUploadBuff()[matNum]->Unmap(0, nullptr);
	}

	// トゥーンテクスチャも同様にマッピング
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetToonUploadBuff()[matNum] == nullptr) continue;

		auto toonSrcAddress = toonImg[matNum]->pixels;
		auto toonrowPitch = Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint8_t* toonmapforImg = nullptr;
		result = bufferHeapCreator->GetToonUploadBuff()[matNum]->Map(0, nullptr, (void**)&toonmapforImg);

		for (int i = 0; i < toonImg[matNum]->height; ++i)
		{
			std::copy_n(toonSrcAddress, toonrowPitch, toonmapforImg);
			toonSrcAddress += toonImg[matNum]->rowPitch;
			toonmapforImg += toonrowPitch;
		}
		
		bufferHeapCreator->GetToonUploadBuff()[matNum]->Unmap(0, nullptr);
	}

	// テクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> src(pmdMaterialInfo->materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> dst(pmdMaterialInfo->materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc(pmdMaterialInfo->materialNum);

	// テクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
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

	// トゥーンテクスチャ用転送オブジェクト
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonSrc(pmdMaterialInfo->materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonDst(pmdMaterialInfo->materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> toonBarriierDesc(pmdMaterialInfo->materialNum);
	// トゥーンテクスチャをGPUのUpload用バッファからGPUのRead用バッファへデータコピー
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

	//行列CBV,SRVディスクリプタヒープ作成
	basicDescHeap = nullptr;
	basicDescHeapDesc = {};
	basicDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	basicDescHeapDesc.NumDescriptors = 1 + pmdMaterialInfo->materialNum * 5; // 行列cbv,material cbv + テクスチャsrv, sph,spa,toon
	basicDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	basicDescHeapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap
	(
		&basicDescHeapDesc,
		IID_PPV_ARGS(basicDescHeap.GetAddressOf())
	);

	//DSVビュー用にディスクリプタヒープ作成
	//ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;

	result = _dev->CreateDescriptorHeap
	(
		&dsvHeapDesc,
		IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())
	);

	// 初期化処理8：各ビューを作成

	vbView = {};
	vbView.BufferLocation = bufferHeapCreator->GetVertBuff()->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = pmdMaterialInfo->vertices.size();//全バイト数
	vbView.StrideInBytes = pmdMaterialInfo->pmdvertex_size;//1頂点あたりのバイト数

	ibView = {};
	ibView.BufferLocation = bufferHeapCreator->GetIdxBuff()->GetGPUVirtualAddress();
	ibView.SizeInBytes = sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size();
	ibView.Format = DXGI_FORMAT_R16_UINT;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {}; // 行列用
	cbvDesc.BufferLocation = matrixBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = matrixBuff->GetDesc().Width;

	D3D12_CONSTANT_BUFFER_VIEW_DESC materialCBVDesc = {}; // マテリアル情報、テクスチャ、sph
	materialCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	materialCBVDesc.SizeInBytes = materialBuffSize;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dev->CreateDepthStencilView
	(
		bufferHeapCreator->GetDepthBuff().Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	//行列用cbv,マテリアル情報用cbv,テクスチャ用srvを順番に生成
	_dev->CreateConstantBufferView
	(
		&cbvDesc,
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()//basicDescHeapHandle
	);

	auto basicDescHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	basicDescHeapHandle.ptr += inc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	// 白テクスチャバッファ
	whiteBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0xff);
	// 黒テクスチャバッファ
	BlackBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0x00);
	// グレーグラデーション
	grayTexBuff = CreateD3DX12ResourceBuffer::CreateGrayGradationTexture(_dev);

	//マテリアル用のcbv,srvを作成
	for (int i = 0; i < pmdMaterialInfo->materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialCBVDesc.BufferLocation += materialBuffSize;

		// テクスチャ
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

		// sphファイル
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

		// spaファイル
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

		// トゥーンテクスチャファイル
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

	//// 初期化処理9：フェンスの生成
	//	ID3D12Fence* _fence = nullptr;
	//	UINT64 _fenceVal = 0;
	//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// 初期化処理10：イベントハンドルの作成
	// 初期化処理11：GPUの処理完了待ち

		//●描画
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

		//以下は不要。_rootSignatureが_pipelineStateに組み込まれており、SetPipe...でまとめてセットされているから。
		//_cmdList->SetGraphicsRootSignature(_rootSignature);

		_cmdList->SetPipelineState(_pipelineState.Get());
		_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature().Get());
		//_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature());
		_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
		_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//現在のバックバッファをインデックスにて取得

		//リソースバリアの準備
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		//リソースバリア：リソースへの複数のアクセスを同期する必要があることをドライバーに通知
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//ハンドルの初期値アドレスにバッファインデックスを乗算し、各ハンドルの先頭アドレスを計算
		handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // auto rtvhでhandleに上書きでも可
		handle.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvh = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->OMSetRenderTargets(1, &handle, true, &dsvh);//レンダーターゲットと深度ステンシルの CPU 記述子ハンドルを設定
		_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // 深度バッファーをクリア
		//画面クリア
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);

		//プリミティブ型に関する情報と、入力アセンブラーステージの入力データを記述するデータ順序をバインド
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//頂点バッファーのCPU記述子ハンドルを設定
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		//インデックスバッファーのビューを設定
		_cmdList->IASetIndexBuffer(&ibView);

		//ディスクリプタヒープ設定および
		//ディスクリプタヒープとルートパラメータの関連付け
		//ここでルートシグネチャのテーブルとディスクリプタが関連付く
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());
		_cmdList->SetGraphicsRootDescriptorTable
		(
			0, // バインドのスロット番号
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		);

		//テキストのように同時に二つの同タイプDHをセットすると、グラボによっては挙動が変化する。
		// 二つ目のセットによりNS300/Hではモデルが表示されなくなった。
		//_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		//_cmdList->SetGraphicsRootDescriptorTable
		//(
		//	1, // バインドのスロット番号
		//	basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		//);

		// マテリアルの
		auto materialHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto materialHInc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		materialHandle.ptr += inc;
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
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//初期化処理：コマンドリストのクローズ(コマンドリストの実行前には必ずクローズする)
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

	delete vertMap;
	delete mappedIdx;
	delete mapMaterial;
	UnregisterClass(prepareRenderingWindow->GetWNDCCLASSEX().lpszClassName, prepareRenderingWindow->GetWNDCCLASSEX().hInstance);

	delete pmdMaterialInfo;
	delete vmdMotionInfo;
	delete pmdActor;
	delete prepareRenderingWindow;
	delete setRootSignature;
	delete gPLSetting;
	delete bufferHeapCreator;
	delete textureLoader;
}