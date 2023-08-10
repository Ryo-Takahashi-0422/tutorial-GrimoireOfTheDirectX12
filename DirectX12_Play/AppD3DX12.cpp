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

// �㏈��
void AppD3DX12::Terminate()
{

};

AppD3DX12::~AppD3DX12()
{

};

HRESULT AppD3DX12::D3DX12DeviceInit()
{
	result = CoInitializeEx(0, COINIT_MULTITHREADED);

	//�t�@�N�g���[�̐���
	result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()))))
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()))))
		{
			return -1;
		}
	}

	//�O���{�������}������Ă���ꍇ�ɃA�_�v�^�[��I�����邽�߂̏���
	//�A�_�v�^�[�̗񋓗p
	std::vector <IDXGIAdapter*> adapters;
	//����̖��O�����A�_�v�^�[�I�u�W�F�N�g������
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
		adpt->GetDesc(&adesc); //�A�_�v�^�[�̐����I�u�W�F�N�g�擾

		//�T�������A�_�v�^�[�̖��O���m�F
		std::wstring strDesc = adesc.Description;
		//printf("%ls\n", strDesc.c_str());
		//printf("%x\n", adesc.DeviceId);

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//�t�B�[�`�����x����
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//Direct3D �f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter.Get(), lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break;//�����\�ȃo�[�W���������������烋�[�v���f
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
	// SetRootSignatureBase�N���X�̃C���X�^���X��
	setRootSignature = new SetRootSignature;

	// SettingShaderCompile�N���X�̃C���X�^���X��
	settingShaderCompile = new SettingShaderCompile;

	// VertexInputLayout�N���X�̃C���X�^���X��
	vertexInputLayout = new VertexInputLayout;
	
	// PMD�t�@�C���̓ǂݍ���
	pmdMaterialInfo = new PMDMaterialInfo;
	if (FAILED(pmdMaterialInfo->ReadPMDHeaderFile(strModelPath))) return false;

	// VMD���[�V�����t�@�C���̓ǂݍ���
	vmdMotionInfo = new VMDMotionInfo;
	if (FAILED(vmdMotionInfo->ReadVMDHeaderFile(strMotionPath))) return false;

	// PMDActor�N���X�̃C���X�^���X��
	pmdActor = new PMDActor(pmdMaterialInfo, vmdMotionInfo);

	// GraphicsPipelineSetting�N���X�̃C���X�^���X��
	gPLSetting = new GraphicsPipelineSetting(vertexInputLayout);

	// �A�j���[�V�����p�̉�]�E���s�ړ��s��̎Q�Ə���
	boneMatrices = new std::vector<DirectX::XMMATRIX>;
	boneMatrices = pmdActor->GetMatrices();
	bNodeTable = pmdMaterialInfo->GetBoneNode();
	
	// �����_�����O�E�B���h�E�ݒ�
	prepareRenderingWindow = new PrepareRenderingWindow;
	prepareRenderingWindow->CreateAppWindow();

	// TextureLoader�N���X�̃C���X�^���X��
	textureLoader = new TextureLoader;

	// BufferHeapCreator�N���X�̃C���X�^���X��
	bufferHeapCreator = new BufferHeapCreator(pmdMaterialInfo, prepareRenderingWindow, textureLoader);

	// TextureTransporter�N���X�̃C���X�^���X��
	textureTransporter = new TextureTransporter(pmdMaterialInfo, bufferHeapCreator);

	// MappingExecuter�N���X�̃C���X�^���X��
	mappingExecuter = new MappingExecuter(pmdMaterialInfo, bufferHeapCreator);

	// ViewCreator�N���X�̃C���X�^���X��
	viewCreator = new ViewCreator(pmdMaterialInfo, bufferHeapCreator);

	// �����_�����O�E�B���h�E�\��
	ShowWindow(prepareRenderingWindow->GetHWND(), SW_SHOW);

	// �r���[�|�[�g�ƃV�U�[�̈�̐ݒ�
	prepareRenderingWindow->SetViewportAndRect();

	// ����߽�֘A�׽�Q
	peraLayout = new PeraLayout;
	peraGPLSetting = new PeraGraphicsPipelineSetting(peraLayout/*vertexInputLayout*/); //TODO PeraLayout,VertexInputLayout�׽�̊��N���X������Ă���ɑΉ�������
	peraPolygon = new PeraPolygon;
	peraSetRootSignature = new PeraSetRootSignature;
	peraShaderCompile = new PeraShaderCompile;
	bufferGPLSetting = new PeraGraphicsPipelineSetting(peraLayout/*vertexInputLayout*/);
	bufferSetRootSignature = new PeraSetRootSignature;
	bufferShaderCompile = new BufferShaderCompile;

	// ���C�g�}�b�v�֘A
	lightMapGPLSetting = new LightMapGraphicsPipelineSetting(vertexInputLayout);
	lightMapRootSignature = new SetRootSignature;
	lightMapShaderCompile = new LightMapShaderCompile;

	// bloom	
	bloomGPLSetting = new PeraGraphicsPipelineSetting(peraLayout);
	bloomRootSignature = new PeraSetRootSignature;
	bloomShaderCompile = new BloomShaderCompile;
}

bool AppD3DX12::PipelineInit(){
//���p�C�v���C���������@�����P�`�V
//�����������P�F�f�o�b�O���C���[���I����
#ifdef _DEBUG
	Utility::EnableDebugLayer();
#endif
//�����������Q�F�e��f�o�C�X�̏����ݒ�
	D3DX12DeviceInit();

	// �J���[�N���A�Ɋւ���Warning���t�B���^�����O(���b�Z�[�W��������Ă��܂�...)
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
		// ���łɃG���[���b�Z�[�W�Ńu���[�N������
		result = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	}

//�����������R�F�R�}���h�L���[�̋L�q�p�ӁE�쐬

		//�R�}���h�L���[�����A�ڍ�obj����
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g����
	cmdQueueDesc.NodeMask = 0;//�A�_�v�^�[��������g��Ȃ��Ƃ���0��OK
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�R�}���h�L���[�̗D��x
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�R�}���h���X�g�ƍ��킹��

	//�R�}���h�L���[����
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));

//�����������S�F�X���b�v�`�F�[���̐���
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
	result = _dxgiFactory->CreateSwapChainForHwnd( //�����Ő���
		_cmdQueue.Get(),
		prepareRenderingWindow->GetHWND(),
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf());

//�����������T�F�����_�[�^�[�Q�b�g�r���[(RTV)�̋L�q�q�q�[�v���쐬
	
	//RTV �L�q�q�q�[�v�̈�̊m��
	bufferHeapCreator->SetRTVHeapDesc();
	//RTV�p�L�q�q�q�[�v�̐����@ID3D12DescriptorHeap�F�L�q�q�̘A�������R���N�V����
	result = bufferHeapCreator->CreateRTVHeap(_dev);

	//�ȉ��̂悤�ɋL�q���邱�ƂŃX���b�v�`�F�[���̎�����V����Desc�I�u�W�F�N�g�ɃR�s�[�ł���
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//�X���b�v�`�F�[���̐���
	//result = _swapChain->GetDesc(&swcDesc);//SWC�̐������擾����

//�����������U�F�t���[�����\�[�X(�e�t���[���̃����_�[�^�[�Q�b�g�r���[)���쐬
	_backBuffers.resize(swapChainDesc.BufferCount); // �ܯ�������ޯ��ޯ̧���ػ���
	handle = bufferHeapCreator->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();//�q�[�v�̐擪��\�� CPU �L�q�q�n���h�����擾

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM/*_SRGB*/;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swapChainDesc.BufferCount; idx++)
	{   //swapEffect DXGI_SWAP_EFFECT_FLIP_DISCARD �̏ꍇ�͍ŏ��̃o�b�t�@�[�̂݃A�N�Z�X�\
		result = _swapChain->GetBuffer(idx, IID_PPV_ARGS(_backBuffers[idx].ReleaseAndGetAddressOf()));//SWC�Ƀo�b�t�@�[��IID�Ƃ���IID�|�C���^��������(SWC�������_�����O���ɃA�N�Z�X����)

		_dev->CreateRenderTargetView//���\�[�X�f�[�^(_backBuffers)�ɃA�N�Z�X���邽�߂̃����_�[�^�[�Q�b�g�r���[��handle�A�h���X�ɍ쐬
		(
			_backBuffers[idx].Get(),//�����_�[�^�[�Q�b�g��\�� ID3D12Resource �I�u�W�F�N�g�ւ̃|�C���^�[
			&rtvDesc,//�����_�[ �^�[�Q�b�g �r���[���L�q���� D3D12_RENDER_TARGET_VIEW_DESC �\���̂ւ̃|�C���^�[�B
			handle//�V�����쐬���ꂽ�����_�[�^�[�Q�b�g�r���[�����݂��鈶���\�� CPU �L�q�q�n���h��(�q�[�v��̃A�h���X)
		);

		//handle���ڽ���L�q�q�̃A�h���X�������L�q�q�T�C�Y���I�t�Z�b�g���Ă���
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

//�����������V�F�R�}���h�A���P�[�^�[���쐬
			//�R�}���h�A���P�[�^�[����>>�R�}���h���X�g�쐬
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	return true;
}

bool AppD3DX12::ResourceInit() {
	//�����\�[�X������

	// ����������1�F���[�g�V�O�l�`���ݒ�
	if (FAILED(setRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// ����߽�p
	if (FAILED(peraSetRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// �\���p
	if (FAILED(bufferSetRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// ���C�g�}�b�v�p
	if (FAILED(lightMapRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}

	// bloom
	if (FAILED(bloomRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}
	
// ����������2�F�V�F�[�_�[�R���p�C���ݒ�
	// _vsBlob��_psBlob�ɼ���ް���ِ߲ݒ�����蓖�Ă�B���ꂼ��̧���߽��ێ����邪�ǂݍ��ݎ��s������nullptr���Ԃ��Ă���B
	auto blobs = settingShaderCompile->SetShaderCompile(setRootSignature, _vsBlob, _psBlob);
	if (blobs.first == nullptr or blobs.second == nullptr) return false;
	_vsBlob = blobs.first;
	_psBlob = blobs.second;	

	// ����߽1���ڗp
	auto mBlobs = peraShaderCompile->SetPeraShaderCompile(peraSetRootSignature, _vsMBlob, _psMBlob);
	if (mBlobs.first == nullptr or mBlobs.second == nullptr) return false;
	_vsMBlob = mBlobs.first;
	_psMBlob = mBlobs.second;

	// �\���p
	auto bufferBlobs = bufferShaderCompile->SetPeraShaderCompile(bufferSetRootSignature, _vsBackbufferBlob, _psBackbufferBlob);
	if (bufferBlobs.first == nullptr or bufferBlobs.second == nullptr) return false;
	_vsBackbufferBlob = bufferBlobs.first;
	_psBackbufferBlob = bufferBlobs.second;

	// ���C�g�}�b�v�p
	auto lightMapBlobs = lightMapShaderCompile->SetShaderCompile(lightMapRootSignature, _lightMapVSBlob, _lightMapPSBlob);
	if (lightMapBlobs.first == nullptr) return false;
	_lightMapVSBlob = lightMapBlobs.first;
	_lightMapPSBlob = lightMapBlobs.second; // �������nullptr

	// bloom
	auto bloomBlobs = bloomShaderCompile->SetPeraShaderCompile(bloomRootSignature, _bloomVSBlob, _bloomPSBlob);
	if (bloomBlobs.first == nullptr or bufferBlobs.second == nullptr) return false;
	_bloomVSBlob = bloomBlobs.first; // �������nullpt
	_bloomPSBlob = bloomBlobs.second;

// ����������3�F���_���̓��C�A�E�g�̍쐬�y��
// ����������4�F�p�C�v���C����ԃI�u�W�F�N�g(PSO)��Desc�L�q���ăI�u�W�F�N�g�쐬
	result = gPLSetting->CreateGPStateWrapper(_dev, setRootSignature, _vsBlob, _psBlob);

	// ����߽�p
	result = peraGPLSetting->CreateGPStateWrapper(_dev, peraSetRootSignature, _vsMBlob, _psMBlob);

	// �\���p
	result = bufferGPLSetting->CreateGPStateWrapper(_dev, bufferSetRootSignature, _vsBackbufferBlob, _psBackbufferBlob);

	// ���C�g�}�b�v�p
	result = lightMapGPLSetting->CreateGPStateWrapper(_dev, lightMapRootSignature, _lightMapVSBlob, _lightMapPSBlob);

	// for shrinked bloom creating
	result = bloomGPLSetting->CreateGPStateWrapper(_dev, bloomRootSignature, _bloomVSBlob, _bloomPSBlob);

// ����������5�F�R�}���h���X�g����
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

// ����������6�F�R�}���h���X�g�̃N���[�Y(�R�}���h���X�g�̎��s�O�ɂ͕K���N���[�Y����)
	//cmdList->Close();

// ����������7�F�e�o�b�t�@�[���쐬���Ē��_����ǂݍ���

	//���_�o�b�t�@�[�̍쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬) 
	result = bufferHeapCreator->CreateBufferOfVertex(_dev);

	//�C���f�b�N�X�o�b�t�@�[���쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬)
	result = bufferHeapCreator->CreateBufferOfIndex(_dev);

	//�f�v�X�o�b�t�@�[���쐬
	result = bufferHeapCreator->CreateBufferOfDepthAndLightMap(_dev);

	//�t�@�C���`�����̃e�N�X�`�����[�h����
	textureLoader->LoadTexture();
	
	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	metaData.resize(pmdMaterialInfo->materialNum);
	img.resize(pmdMaterialInfo->materialNum);
	ScratchImage scratchImg = {};
	bufferHeapCreator->CreateUploadAndReadBuff4PmdTexture(_dev, strModelPath, metaData, img); // �o�b�t�@�쐬
	
	// �g�D�[���e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	toonMetaData.resize(pmdMaterialInfo->materialNum);
	toonImg.resize(pmdMaterialInfo->materialNum);
	bufferHeapCreator->CreateToonUploadAndReadBuff(_dev, strModelPath, toonMetaData, toonImg); // �o�b�t�@�쐬

	//�s��p�萔�o�b�t�@�[�̐���
	pmdMaterialInfo->worldMat = XMMatrixIdentity();

	//auto worldMat = XMMatrixRotationY(15.0f);
	pmdMaterialInfo->angle = 0.0f;

	//�r���[�s��̐����E��Z
	XMFLOAT3 eye(0, 15, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	auto viewMat = XMMatrixLookAtLH
	(
		XMLoadFloat3(&eye),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up)
	);

	//�v���W�F�N�V����(�ˉe)�s��̐����E��Z
	auto projMat = XMMatrixPerspectiveFovLH
	(
		XM_PIDIV2, // ��p90��
		static_cast<float>(prepareRenderingWindow->GetWindowHeight()) / static_cast<float>(prepareRenderingWindow->GetWindowWidth()),
		1.0, // �j�A�\�N���b�v
		100.0 // �t�@�[�N���b�v
	);

	// �s��p�萔�o�b�t�@�[�̐���
	result = bufferHeapCreator->CreateConstBufferOfWVPMatrix(_dev);
	
	//�}�e���A���p�萔�o�b�t�@�[�̐���
	result = bufferHeapCreator->CreateConstBufferOfMaterial(_dev);

	// �}���`�p�X�����_�����O�p�ɏ������ݐ惊�\�[�X�̍쐬
    // �쐬�ς݂̃q�[�v�����g���Ă����ꖇ�����_�����O���p��
	// �g���Ă���o�b�N�o�b�t�@�[�̏��𗘗p����
	auto& bbuff = _backBuffers[0];
	auto mutipassResDesc = bbuff->GetDesc();
	// RTV,SRV�p�o�b�t�@�[�Ɗe�q�[�v�쐬
	result = bufferHeapCreator->CreateRenderBufferForMultipass(_dev, mutipassResDesc);
	bufferHeapCreator->CreateMultipassRTVHeap(_dev);
	bufferHeapCreator->CreateMultipassSRVHeap(_dev);

	//���_�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɒ��_�f�[�^���R�s�[����B
	mappingExecuter->MappingVertBuff();

	//�C���f�N�X�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	mappingExecuter->MappingIndexOfVertexBuff();

	//�s��p�萔�o�b�t�@�[�̃}�b�s���O
	// ���ʁA���C�g���W
	_planeNormalVec = XMFLOAT4(0, 1, 0, 0);
	lightVec = XMFLOAT3(-0.5f, 1, -0.5f);
	auto light = XMLoadFloat3(&lightVec);
	auto eyePos = XMLoadFloat3(&eye);
	auto targetPos = XMLoadFloat3(&target);
	auto upVec = XMLoadFloat3(&up);
	light = targetPos + XMVector3Normalize(light) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];

	result = bufferHeapCreator->GetMatrixBuff()->Map(0, nullptr, (void**)&pmdMaterialInfo->mapMatrix);
	pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;
	pmdMaterialInfo->mapMatrix->view = viewMat;
	pmdMaterialInfo->mapMatrix->proj = projMat;
	pmdMaterialInfo->mapMatrix->lightCamera = XMMatrixLookAtLH(light, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);
	pmdMaterialInfo->mapMatrix->shadow = XMMatrixShadow(XMLoadFloat4(&_planeNormalVec), -XMLoadFloat3(&lightVec));
	pmdMaterialInfo->mapMatrix->eye = eye;

	// ���Ɠ������Ƃ����Ă���B���C�g�}�b�v�p�ɂ�mapMatrix�Ƀ}�b�s���O�����烂�f����������BTODO:�����ĂȂ��̂łȂ�Ƃ�������...
	result = bufferHeapCreator->GetMatrixBuff4Multipass()->Map(0, nullptr, (void**)&pmdMaterialInfo->mapMatrix4Lightmap); // ���C�g�}�b�v�p�ɂ��}�b�s���O
	pmdMaterialInfo->mapMatrix4Lightmap->world = pmdMaterialInfo->worldMat;
	pmdMaterialInfo->mapMatrix4Lightmap->view = viewMat;
	pmdMaterialInfo->mapMatrix4Lightmap->proj = projMat;
	pmdMaterialInfo->mapMatrix4Lightmap->lightCamera = XMMatrixLookAtLH(light, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.0f, 100.0f);
	pmdMaterialInfo->mapMatrix4Lightmap->shadow = XMMatrixShadow(XMLoadFloat4(&_planeNormalVec), -XMLoadFloat3(&lightVec));
	pmdMaterialInfo->mapMatrix4Lightmap->eye = eye;

	//�}�e���A���p�o�b�t�@�[�ւ̃}�b�s���O
	mappingExecuter->MappingMaterialBuff();

	// TODO:strMoodelPath�����X�g�����Ă܂Ƃ߂ď����o����悤�ɂ���BBufferHeapCreator��MappngExecuter�����点��B
	// �m�[�}���}�b�v�ǂݍ��݁A�o�b�t�@�쐬�A�}�b�s���O
	bufferHeapCreator->CreateUploadAndReadBuff4Normalmap(_dev, strModelPath, "jpg", 1);
	mappingExecuter->TransferTexUploadToBuff(bufferHeapCreator->GetNormalMapUploadBuff(), bufferHeapCreator->GetNormalMapImg(), 1);
	textureTransporter->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue, 
		bufferHeapCreator->GetNormalMapMetadata(), bufferHeapCreator->GetNormalMapImg(),
		_fence, _fenceVal, bufferHeapCreator->GetNormalMapUploadBuff(), bufferHeapCreator->GetNormalMapReadBuff(), 1);
	
	// �e�N�X�`���̃A�b�v���[�h�p�o�b�t�@�ւ̃}�b�s���O
	mappingExecuter->TransferTexUploadToBuff(bufferHeapCreator->GetPMDTexUploadBuff(), img, pmdMaterialInfo->materialNum);
	// �e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
	textureTransporter->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue, metaData, img,
		_fence, _fenceVal, bufferHeapCreator->GetPMDTexUploadBuff(), bufferHeapCreator->GetPMDTexReadBuff(), pmdMaterialInfo->materialNum);

	// �g�D�[���e�N�X�`�������l�Ƀ}�b�s���O
	mappingExecuter->TransferTexUploadToBuff(bufferHeapCreator->GetToonUploadBuff(), toonImg, pmdMaterialInfo->materialNum);
	// �g�D�[���e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
	textureTransporter->TransportPMDMaterialTexture(_cmdList, _cmdAllocator, _cmdQueue, toonMetaData, toonImg,
		_fence, _fenceVal, bufferHeapCreator->GetToonUploadBuff(), bufferHeapCreator->GetToonReadBuff(), pmdMaterialInfo->materialNum);

	//CBV,SRV�f�B�X�N���v�^�q�[�v�쐬(�s��A�e�N�X�`���ɗ��p)
	result = bufferHeapCreator->CreateCBVSRVHeap(_dev);

	//DSV�r���[�p�Ƀf�B�X�N���v�^�q�[�v�쐬
	result = bufferHeapCreator->CreateDSVHeap(_dev);

// ����������8�F�e�r���[���쐬

	// Vertex�r���[�쐬
	viewCreator->CreateVertexBufferView();

	// Index�r���[�쐬
	viewCreator->CreateIndexBufferView();

	// DSV�쐬
	viewCreator->CreateDSVWrapper(_dev);

	// �s��pcbv�쐬
	viewCreator->CreateCBV4Matrix(_dev);
	// pmd���f���̃}�e���A���A�e�N�X�`���Asph�p�r���[���쐬�B���ꂪ�Ȃ��ƃ��f���^�����ɂȂ�B
	viewCreator->CreateCBVSRV4MateriallTextureSph(_dev);

	// �K�E�V�A���ڂ����p�E�F�C�g�A�o�b�t�@�[�쐬�A�}�b�s���O�A�f�B�X�N���v�^�q�[�v�쐬�A�r���[�쐬�܂�
	auto weights = Utility::GetGaussianWeight(8, 5.0f);
	bufferHeapCreator->CreateConstBufferOfGaussian(_dev, weights);
	mappingExecuter->MappingGaussianWeight(weights);
	//bufferHeapCreator->CreateEffectHeap(_dev);
	//viewCreator->CreateCBV4GaussianView(_dev);

	// �}���`�p�X�p�r���[�쐬
	peraPolygon->CreatePeraView(_dev);
	viewCreator->CreateRTV4Multipasses(_dev);
	viewCreator->CreateSRV4Multipasses(_dev);

// ����������9�F�t�F���X�̐���
	//	ID3D12Fence* _fence = nullptr;
	//	UINT64 _fenceVal = 0;
	//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

// ����������10�F�C�x���g�n���h���̍쐬
// ����������11�FGPU�̏��������҂�




	return true;
}

void AppD3DX12::Run() {
	MSG msg = {};
	pmdActor->PlayAnimation(); // �A�j���[�V�����J�n�����̎擾
	pmdActor->MotionUpdate(_duration);
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�A�v���I������message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
		{
			printf("%s", "quit");
			break;
		}

		auto dsvh = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

		DrawLightMap(); // draw lightmap
		
		DrawPeraPolygon(); // draw background polygon

		DrawModel(); // draw pmd model

		DrawShrinkTextureForBlur(); // draw shrink buffer

		DrawBackBuffer(); // draw back buffer

		//�R�}���h���X�g�̃N���[�Y(�R�}���h���X�g�̎��s�O�ɂ͕K���N���[�Y����)
		_cmdList->Close();

		//�R�}���h�L���[�̎��s
		ID3D12CommandList* cmdLists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//ID3D12Fence��Signal��CPU���̃t�F���X�ő������s
		//ID3D12CommandQueue��Signal��GPU���̃t�F���X��
		//�R�}���h�L���[�ɑ΂��鑼�̂��ׂĂ̑��삪����������Ƀt�F���X�X�V
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

		while (_fence->GetCompletedValue() != _fenceVal)
		{
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			//�C�x���g�����҂�
			WaitForSingleObject(event, INFINITE);
			//�C�x���g�n���h�������
			CloseHandle(event);
		}

		_cmdAllocator->Reset();//�R�}���h �A���P�[�^�[�Ɋ֘A�t�����Ă��郁�������ė��p����
		_cmdList->Reset(_cmdAllocator.Get(), nullptr);//�R�}���h���X�g���A�V�����R�}���h���X�g���쐬���ꂽ���̂悤�ɏ�����ԂɃ��Z�b�g

		//�s����̍X�V
		//pmdMaterialInfo->angle += 0.01f;
		//pmdMaterialInfo->angle = 200.0f;
		pmdMaterialInfo->worldMat = XMMatrixRotationY(pmdMaterialInfo->angle);
		pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;

		// ���[�V�����p�s��̍X�V�Ə�������
		pmdActor->MotionUpdate(pmdActor->GetDuration());
		pmdActor->UpdateVMDMotion();
		//pmdActor->RecursiveMatrixMultiply(XMMatrixIdentity());
		//pmdActor->IKSolve();
		std::copy(boneMatrices->begin(), boneMatrices->end(), pmdMaterialInfo->mapMatrix->bones);

		//�t���b�v���ă����_�����O���ꂽ�C���[�W�����[�U�[�ɕ\��
		_swapChain->Present(1, 0);		
	}

	delete bufferGPLSetting;
	delete bufferShaderCompile;
	
	delete lightMapGPLSetting;	
	delete lightMapShaderCompile;
	
	delete viewCreator;
	delete mappingExecuter;
	UnregisterClass(prepareRenderingWindow->GetWNDCCLASSEX().lpszClassName, prepareRenderingWindow->GetWNDCCLASSEX().hInstance);

	delete bufferHeapCreator;
	delete textureTransporter;
	delete textureLoader;

	delete pmdActor;
	delete settingShaderCompile;
	delete gPLSetting;

	delete vmdMotionInfo;
	delete prepareRenderingWindow;
	delete pmdMaterialInfo;

	delete peraGPLSetting;
	delete peraLayout;
	delete peraPolygon;
	delete peraShaderCompile;	

	//delete bufferSetRootSignature;
	//delete lightMapRootSignature;
	//delete setRootSignature;
	//delete peraSetRootSignature;
}

void AppD3DX12::DrawLightMap()
{
	constexpr uint32_t shadow_difinition = 1024;
	D3D12_VIEWPORT vp = CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);
	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);

	auto dsvh = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();

	_cmdList->SetPipelineState(lightMapGPLSetting->GetPipelineState().Get());
	_cmdList->SetGraphicsRootSignature(lightMapRootSignature->GetRootSignature().Get());

	dsvh.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvh);
	_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // �[�x�o�b�t�@�[���N���A
	//��ʃN���A
	//_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, viewCreator->GetVbView());
	_cmdList->IASetIndexBuffer(viewCreator->GetIbView());

	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->GetCBVSRVHeap().GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable
	(
		0, // �o�C���h�̃X���b�g�ԍ�
		bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
	);

	auto materialHandle2 = bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	auto inc2 = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto materialHInc2 = inc2 * 5; // �s��cbv + (material cbv+�e�N�X�`��srv+sph srv+spa srv+toon srv)
	materialHandle2.ptr += inc2; // ���̏����̒��O�ɍs��pCBV������ؽĂɃZ�b�g��������
	unsigned int idxOffset2 = 0;

	for (auto m : pmdMaterialInfo->materials)
	{
		_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle2);
		//�C���f�b�N�X�t���C���X�^���X�����ꂽ�v���~�e�B�u��`��
		_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset2, 0, 0); // instanceid 0:�ʏ�A1:�e

		materialHandle2.ptr += materialHInc2;
		idxOffset2 += m.indiceNum;
	}

	_cmdList->DrawIndexedInstanced(pmdMaterialInfo->vertNum, 1, 0, 0, 0);

	// ���C�g�}�b�v��Ԃ������ݸ����ޯĂɕύX����
	D3D12_RESOURCE_BARRIER barrierDesc4LightMap = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetLightMapBuff().Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4LightMap);
}

void AppD3DX12::DrawPeraPolygon()
{
	//// �}���`�p�X1�p�X��

	D3D12_RESOURCE_BARRIER barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetMultipassBuff().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Multi);

	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

	auto rtvHeapPointer = bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	_cmdList->SetGraphicsRootSignature(peraSetRootSignature->GetRootSignature().Get());
	// no need SetDescriptorHeaps, SetGraphicsRootDescriptorTable, because it only needs rendering.

	_cmdList->SetPipelineState(peraGPLSetting->GetPipelineState().Get());
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, peraPolygon->GetVBView());
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// ����߽ؿ����ر���ɖ߂�
	barrierDesc4Multi = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetMultipassBuff().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Multi);
}

void AppD3DX12::DrawModel()
{
	//���\�[�X�o���A�̏����B�ܯ�������ޯ��ޯ̧��..._COMMON��������ԂƂ��錈�܂�B�����color
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = bufferHeapCreator->GetMultipassBuff2().Get();
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//���\�[�X�o���A�F���\�[�X�ւ̕����̃A�N�Z�X�𓯊�����K�v�����邱�Ƃ��h���C�o�[�ɒʒm
	_cmdList->ResourceBarrier(1, &BarrierDesc);


	// normal
	D3D12_RESOURCE_BARRIER barrierDesc4test = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetMultipassBuff3().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4test);

	// bloom
	D3D12_RESOURCE_BARRIER barrierDesc4Bloom = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetBloomBuff()[0].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Bloom);


	// ���f���`��
	_cmdList->SetPipelineState(gPLSetting->GetPipelineState().Get());
	_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature().Get());
	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

	//�n���h���̏����l�A�h���X�Ƀo�b�t�@�C���f�b�N�X����Z���A�e�n���h���̐擪�A�h���X���v�Z
	//handle = bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart(); // auto rtvh��handle�ɏ㏑���ł���
	//handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//auto handle2 = handle;
	//handle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] =
	//{
	//	handle,handle2
	//};

	auto dsvh = bufferHeapCreator->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[3];
	auto baseH = bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	uint32_t offset = 1; // start from No.2 RTV
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseH, incSize * offset);
		offset += 1;
	}
	_cmdList->OMSetRenderTargets(3, handles, false, &dsvh);

	// �����_�[�^�[�Q�b�g�Ɛ[�x�X�e���V��(�����V�F�[�_�[���F���o���Ȃ��r���[)��CPU�L�q�q�n���h����ݒ肵�ăp�C�v���C���ɒ��o�C���h
	// �Ȃ̂ł��̓��ނ̃r���[�̓}�b�s���O���Ȃ�����
	//_cmdList->OMSetRenderTargets(2, rtvs/*&handle*/, false, &dsvh);
	_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // �[�x�o�b�t�@�[���N���A

	//��ʃN���A
	float clearColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	_cmdList->ClearRenderTargetView(handles[0], clearColor, 0, nullptr);
	_cmdList->ClearRenderTargetView(handles[1], clearColor, 0, nullptr);
	clearColor[0] = 0;
	clearColor[1] = 0;
	clearColor[2] = 0;
	_cmdList->ClearRenderTargetView(handles[2], clearColor, 0, nullptr);

	//�v���~�e�B�u�^�Ɋւ�����ƁA���̓A�Z���u���[�X�e�[�W�̓��̓f�[�^���L�q����f�[�^�������o�C���h
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//���_�o�b�t�@�[��CPU�L�q�q�n���h����ݒ�
	_cmdList->IASetVertexBuffers(0, 1, viewCreator->GetVbView());

	//�C���f�b�N�X�o�b�t�@�[�̃r���[��ݒ�
	_cmdList->IASetIndexBuffer(viewCreator->GetIbView());

	//�f�B�X�N���v�^�q�[�v�ݒ肨���
	//�f�B�X�N���v�^�q�[�v�ƃ��[�g�p�����[�^�̊֘A�t��
	//�����Ń��[�g�V�O�l�`���̃e�[�u���ƃf�B�X�N���v�^���֘A�t��
	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->GetCBVSRVHeap().GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable
	(
		0, // �o�C���h�̃X���b�g�ԍ�
		bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
	);

	//////�e�L�X�g�̂悤�ɓ����ɓ�̓��^�C�vDH���Z�b�g����ƁA�O���{�ɂ���Ă͋������ω�����B
	////// ��ڂ̃Z�b�g�ɂ��NS300/H�ł̓��f�����\������Ȃ��Ȃ����B
	//////_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
	//////_cmdList->SetGraphicsRootDescriptorTable
	//////(
	//////	1, // �o�C���h�̃X���b�g�ԍ�
	//////	bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart()
	//////);

	// �}�e���A���̃f�B�X�N���v�^�q�[�v�����[�g�V�O�l�`���̃e�[�u���Ƀo�C���h���Ă���
	// CBV:1��(matrix)�ASRV:4��(colortex, graytex, spa, sph)���ΏہBSetRootSignature.cpp�Q�ƁB
	auto materialHandle = bufferHeapCreator->GetCBVSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto materialHInc = inc * 5; // �s��cbv + (material cbv+�e�N�X�`��srv+sph srv+spa srv+toon srv)
	materialHandle.ptr += inc; // ���̏����̒��O�ɍs��pCBV������ؽĂɃZ�b�g��������
	unsigned int idxOffset = 0;

	// (���Ԃ�)DrawIndexedInstanced�ɂ��`��̑O��SRV����̃e�N�X�`���擾���I���Ă��Ȃ��ƃf�[�^���V�F�[�_�[�ɒʂ�Ȃ�
	// �Ȃ��A���̃p�X�ł̃f�v�X���`��Ɠ����ɓn���Ă��邪�Q�Əo���Ȃ��̂́A���\�[�X��Ԃ�depth_write�̂܂܂�����Ǝv����
	_cmdList->SetGraphicsRootDescriptorTable(2, materialHandle); // �f�v�X�}�b�v�i�[
	materialHandle.ptr += inc;
	_cmdList->SetGraphicsRootDescriptorTable(3, materialHandle); // ���C�g�}�b�v�i�[
	materialHandle.ptr += inc;

	for (auto m : pmdMaterialInfo->materials)
	{
		_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
		//�C���f�b�N�X�t���C���X�^���X�����ꂽ�v���~�e�B�u��`��
		_cmdList->DrawIndexedInstanced(m.indiceNum, 2, idxOffset, 0, 0); // instanceid 0:�ʏ�A1:�e

		materialHandle.ptr += materialHInc;
		idxOffset += m.indiceNum;
	}

	// color
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// normal
	barrierDesc4test = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetMultipassBuff3().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4test);

	// bloom
	barrierDesc4Bloom = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetBloomBuff()[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Bloom);
}


void AppD3DX12::DrawShrinkTextureForBlur()
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
		bufferHeapCreator->GetBloomBuff()[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4Shrink);

	// model blur renderer status to
	D3D12_RESOURCE_BARRIER barrierDesc4ShrinkModel = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetBloomBuff()[2].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4ShrinkModel);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[2];
	auto baseH = bufferHeapCreator->GetMultipassRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	//baseH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 4;
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

	_cmdList->SetDescriptorHeaps(1, bufferHeapCreator->GetMultipassSRVHeap().GetAddressOf());

	// bloom texture
	auto srvHandle = bufferHeapCreator->GetMultipassSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	srvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // model texture
	_cmdList->SetGraphicsRootDescriptorTable(1, srvHandle);
	srvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // gaussian value
	_cmdList->SetGraphicsRootDescriptorTable(2, srvHandle); // table[2] is for gaussian value
	srvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 6; // bloom texture
	_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);

	auto desc = bufferHeapCreator->GetBloomBuff()[0]->GetDesc();
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

void AppD3DX12::DrawBackBuffer()
{

	auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//���݂̃o�b�N�o�b�t�@���C���f�b�N�X�ɂĎ擾

	// �f�v�X�}�b�v�p�o�b�t�@�̏�Ԃ�ǂݍ��݉\�ɕς���
	D3D12_RESOURCE_BARRIER barrierDesc4DepthMap = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->/*GetDepthMapBuff*/GetDepthBuff().Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer()); // ���͏d�v
	_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer()); // ���͏d�v

	// �ޯ��ޯ̧�ɕ`�悷��
	// �ޯ��ޯ̧��Ԃ������ݸ����ޯĂɕύX����
	D3D12_RESOURCE_BARRIER barrierDesc4BackBuffer = CD3DX12_RESOURCE_BARRIER::Transition
	(
		_backBuffers[bbIdx].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);

	auto rtvHeapPointer = bufferHeapCreator->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, /*&dsvh*/nullptr);
	//_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // �[�x�o�b�t�@�[���N���A

	float clsClr[4] = { 0.2,0.5,0.5,1.0 };
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clsClr, 0, nullptr);

	// �쐬����ø����̗��p����
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
	_cmdList->SetGraphicsRootDescriptorTable(2, gHandle2); // �ڂ����萔

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(3, gHandle2); // �@���}�b�v

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(4, gHandle2); // �[�x�}�b�v

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(5, gHandle2); // ���C�g�}�b�v

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(6, gHandle2); // ���C�g�}�b�v�p�V�[���s��

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(7, gHandle2); // �}���`�^�[�Q�b�g�`��

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(8, gHandle2); // bloom

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(9, gHandle2); // shrinked bloom

	gHandle2.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(10, gHandle2); // shrinked model

	_cmdList->DrawInstanced(4, 1, 0, 0);

	// �ޯ��ޯ̧��Ԃ������ݸ����ޯĂ��猳�ɖ߂�
	barrierDesc4BackBuffer.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc4BackBuffer.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &barrierDesc4BackBuffer);

	// �f�v�X�}�b�v�p�o�b�t�@�̏�Ԃ��������݉\�ɖ߂�
	barrierDesc4DepthMap = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->/*GetDepthMapBuff*/GetDepthBuff().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	// ���C�g�}�b�v�p�o�b�t�@�̏�Ԃ��������݉\�ɖ߂�
	auto barrierDesc4LightMap = CD3DX12_RESOURCE_BARRIER::Transition
	(
		bufferHeapCreator->GetLightMapBuff().Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE

	);
	_cmdList->ResourceBarrier(1, &barrierDesc4LightMap);
}