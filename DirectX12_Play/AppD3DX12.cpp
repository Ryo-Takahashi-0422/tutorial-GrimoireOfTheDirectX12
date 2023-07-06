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

#ifdef _DEBUG
bool AppD3DX12::PrepareRendering() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	// PMD�t�@�C���̓ǂݍ���
	pmdMaterialInfo = new PMDMaterialInfo;
	if (FAILED(pmdMaterialInfo->ReadPMDHeaderFile(strModelPath))) return false;

	// VMD���[�V�����t�@�C���̓ǂݍ���
	vmdMotionInfo = new VMDMotionInfo;
	if (FAILED(vmdMotionInfo->ReadVMDHeaderFile(strMotionPath))) return false;

	// PMDActor�N���X�̃C���X�^���X��
	pmdActor = new PMDActor(pmdMaterialInfo, vmdMotionInfo);
	//pmdActor->SolveCCDIK(pmdMaterialInfo->GetpPMDIKData()[0]);

	// GraphicsPipelineSetting�N���X�̃C���X�^���X��
	gPLSetting = new GraphicsPipelineSetting;

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

	// �����_�����O�E�B���h�E�\��
	ShowWindow(prepareRenderingWindow->GetHWND(), SW_SHOW);

	// �r���[�|�[�g�ƃV�U�[�̈�̐ݒ�
	prepareRenderingWindow->SetViewportAndRect();
}

bool AppD3DX12::PipelineInit(){
//���p�C�v���C���������@�����P�`�V
//�����������P�F�f�o�b�O���C���[���I����
#ifdef _DEBUG
	Utility::EnableDebugLayer();
#endif
	// �e��f�o�C�X�̏����ݒ�
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

	result = infoQueue->PushStorageFilter(&filter);
	// ���łɃG���[���b�Z�[�W�Ńu���[�N������
	result = infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

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
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	//�L�q�q�q�[�v�̐����@ID3D12DescriptorHeap�F�L�q�q�̘A�������R���N�V����
	//ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));

	//�ȉ��̂悤�ɋL�q���邱�ƂŃX���b�v�`�F�[���̎�����V����Desc�I�u�W�F�N�g�ɃR�s�[�ł���
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//�X���b�v�`�F�[���̐���
	//result = _swapChain->GetDesc(&swcDesc);//SWC�̐������擾����

//�����������U�F�t���[�����\�[�X(�e�t���[���̃����_�[�^�[�Q�b�g�r���[)���쐬
	_backBuffers.resize(swapChainDesc.BufferCount);//���\�[�X�o�b�t�@�[
	handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();//�q�[�v�̐擪��\�� CPU �L�q�q�n���h�����擾

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
	////�����\�[�X������
	
	//// ����������1�F���[�g�V�O�l�`���ݒ�
	setRootSignature = new SetRootSignature;
	if (FAILED(setRootSignature->SetRootsignatureParam(_dev)))
	{
		return false;
	}
	
	setRootSignature->SetRootsignatureParam(_dev);

	// ����������2�F�V�F�[�_�[�R���p�C��

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

	//�G���[�`�F�b�N
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C����������܂���");
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

	// ����������3�F���_���̓��C�A�E�g�̍쐬

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		//���W
		{
			"POSITION",
			0, // �����Z�}���e�B�N�X�ɑ΂���C���f�b�N�X
			DXGI_FORMAT_R32G32B32_FLOAT,
			0, // �X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0 // ��x�ɕ`�悷��C���X�^���X��
		},

		//�@���x�N�g��
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
			0, // �X���b�g�C���f�b�N�X
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//�{�[���ԍ�
		{
			"BONE_NO",
			0,
			DXGI_FORMAT_R16G16_UINT, // bone[0], bone[1]
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//�@���x�N�g��
		{
			"WEIGHT",
			0,
			DXGI_FORMAT_R8_UINT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},

		//�@���x�N�g��
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


	// ����������4�F�p�C�v���C����ԃI�u�W�F�N�g(PSO)��Desc�L�q���ăI�u�W�F�N�g�쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeLine = {};
	for(int i=0;i<sizeof(inputLayout);++i)
	{
		gPLSetting->SetInputlayout(i, inputLayout[i]);
	}

	gpipeLine = gPLSetting->SetGPL(gpipeLine, _dev, _pipelineState, setRootSignature, _vsBlob, _psBlob);
	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// ����������5�F�R�}���h���X�g����
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	// ����������6�F�R�}���h���X�g�̃N���[�Y(�R�}���h���X�g�̎��s�O�ɂ͕K���N���[�Y����)
	//cmdList->Close();

	// ����������7�F�e�o�b�t�@�[���쐬���Ē��_����ǂݍ���

	//���_�o�b�t�@�[�ƃC���f�b�N�X�o�b�t�@�[�p�̃q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES heapProps = {};
	bufferHeapCreator->SetVertexAndIndexHeapProp(&heapProps);

	//�[�x�o�b�t�@�[�p�q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	bufferHeapCreator->SetDepthHeapProp(&depthHeapProps);

	//�[�x�o�b�t�@�[�p���\�[�X�f�B�X�N���v�^
	D3D12_RESOURCE_DESC depthResDesc = {};
	bufferHeapCreator->SetDepthResourceDesc(&depthResDesc);

	//�N���A�o�����[(����̃��\�[�X�̃N���A������œK�����邽�߂Ɏg�p�����l)
	D3D12_CLEAR_VALUE depthClearValue = {};
	bufferHeapCreator->SetClearValue(&depthClearValue);

	//ID3D12Resource�I�u�W�F�N�g�̓����p�����[�^�ݒ�
	D3D12_RESOURCE_DESC vertresDesc = CD3DX12_RESOURCE_DESC::Buffer(pmdMaterialInfo->vertices.size());
	D3D12_RESOURCE_DESC indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size());
	
	//���_�o�b�t�@�[�̍쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬) 
	result = bufferHeapCreator->CreateBufferOfVertex(_dev, heapProps, vertresDesc);

	//�C���f�b�N�X�o�b�t�@�[���쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬)
	result = bufferHeapCreator->CreateBufferOfIndex(_dev, heapProps, indicesDesc);

	//�f�v�X�o�b�t�@�[���쐬
	result = bufferHeapCreator->CreateBufferOfDepth(_dev, depthHeapProps, depthResDesc);

	//�t�@�C���`�����̃e�N�X�`�����[�h����
	textureLoader->LoadTexture();
	
	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	metaData.resize(pmdMaterialInfo->materialNum);
	img.resize(pmdMaterialInfo->materialNum);
	ScratchImage scratchImg = {};
	result = CoInitializeEx(0, COINIT_MULTITHREADED);	
	bufferHeapCreator->CreateUploadAndReadBuff(_dev, strModelPath, metaData, img); // �o�b�t�@�쐬

	// �g�D�[���e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	std::string toonFilePath = "toon\\";
	struct _stat s = {};
	toonMetaData.resize(pmdMaterialInfo->materialNum);
	toonImg.resize(pmdMaterialInfo->materialNum);
	ScratchImage toonScratchImg = {};
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
	
	//�}�e���A���p�萔�o�b�t�@�[�̐���
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

	// �}���`�p�X�����_�����O�p
    // �쐬�ς݂̃q�[�v�����g���Ă����ꖇ�����_�����O���p��
	auto heapDesc2 = rtvHeaps->GetDesc();

	// �g���Ă���o�b�N�o�b�t�@�[�̏��𗘗p����
	auto& bbuff = _backBuffers[0];
	auto resDesc2 = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// �����_�����O���̃N���A�l�Ɠ����l
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

	//���_�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɒ��_�f�[�^���R�s�[����B
	//CPU�͈ÖٓI�ȃq�[�v�̏��𓾂��Ȃ����߁AMap�֐��ɂ��VRAM��̃o�b�t�@�[�ɃA�h���X�����蓖�Ă���Ԃ�
	//���_�Ȃǂ̏���VRAM�փR�s�[���Ă���(���̂R��CPU��GPU�ǂ�����A�N�Z�X�\��UPLOAD�^�C�v�ȃq�[�v�̃}�b�v�\)�A
	//s�Ƃ��������BUnmap�̓R�����g�A�E�g���Ă����ɉe���͂Ȃ���...
	//vertMap = nullptr;
	result = bufferHeapCreator->GetVertBuff()->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(pmdMaterialInfo->vertices), std::end(pmdMaterialInfo->vertices), vertMap);
	bufferHeapCreator->GetVertBuff()->Unmap(0, nullptr);

	//�C���f�N�X�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	//mappedIdx = nullptr;
	result = bufferHeapCreator->GetIdxBuff()->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(pmdMaterialInfo->indices), std::end(pmdMaterialInfo->indices), mappedIdx);
	bufferHeapCreator->GetIdxBuff()->Unmap(0, nullptr);

	//boneMatrices = pmdMaterialInfo->GetBoneMatrices();
	//�s��p�萔�o�b�t�@�[�̃}�b�s���O
	//mapMatrix = nullptr;
	result = matrixBuff->Map(0, nullptr, (void**)&pmdMaterialInfo->mapMatrix);
	pmdMaterialInfo->mapMatrix->world = pmdMaterialInfo->worldMat;
	pmdMaterialInfo->mapMatrix->view = viewMat;
	pmdMaterialInfo->mapMatrix->proj = projMat;
	pmdMaterialInfo->mapMatrix->eye = eye;

	//�}�e���A���p�o�b�t�@�[�ւ̃}�b�s���O
	//mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto m : pmdMaterialInfo->materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}
	materialBuff->Unmap(0, nullptr);

	// �e�N�X�`���A�b�v���[�h�p�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA
	// ���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	// �e�N�X�`���̃A�b�v���[�h�p�o�b�t�@�ւ̃}�b�s���O
	for (int matNum = 0; matNum < pmdMaterialInfo->materialNum; matNum++)
	{
		if (bufferHeapCreator->GetTexUploadBuff()[matNum] == nullptr) continue;

		auto srcAddress = img[matNum]->pixels;
		auto rowPitch = Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);//////////////////////////////
		uint8_t* mapforImg = nullptr; // �s�N�Z���f�[�^�̃V�X�e���������o�b�t�@�[�ւ̃|�C���^�[��uint8_t(img->pixcel)
		result = bufferHeapCreator->GetTexUploadBuff()[matNum]->Map(0, nullptr, (void**)&mapforImg);

		// img:���f�[�^�̏����A�h���X(srcAddress)�����s�b�`���I�t�Z�b�g���Ȃ���A�␳�����s�b�`��(rowPitch)�̃A�h���X��
		// mapforImg�ɂ��̐���(rowPitch)�I�t�Z�b�g���J��Ԃ��R�s�[���Ă���
		//std::copy_n(img->pixels, img->slicePitch, mapforImg);
		for (int i = 0; i < img[matNum]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[matNum]->rowPitch;
			mapforImg += rowPitch;
		}

		bufferHeapCreator->GetTexUploadBuff()[matNum]->Unmap(0, nullptr);
	}

	// �g�D�[���e�N�X�`�������l�Ƀ}�b�s���O
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

	// �e�N�X�`���p�]���I�u�W�F�N�g
	std::vector<D3D12_TEXTURE_COPY_LOCATION> src(pmdMaterialInfo->materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> dst(pmdMaterialInfo->materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc(pmdMaterialInfo->materialNum);

	// �e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
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
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//�R�s�[��ݒ�
		dst[matNum].pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//�o���A�ݒ�
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetTexReadBuff()[matNum].Get();
			texBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[matNum]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator.Get(), nullptr);
		}
	}

	// �g�D�[���e�N�X�`���p�]���I�u�W�F�N�g
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonSrc(pmdMaterialInfo->materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonDst(pmdMaterialInfo->materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> toonBarriierDesc(pmdMaterialInfo->materialNum);
	// �g�D�[���e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
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
			Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//�R�s�[��ݒ�
		toonDst[matNum].pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//�o���A�ݒ�
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = bufferHeapCreator->GetToonReadBuff()[matNum].Get();
			toonBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toonBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			toonBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &toonBarriierDesc[matNum]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator.Get(), nullptr);
		}
	}

	//�s��CBV,SRV�f�B�X�N���v�^�q�[�v�쐬
	basicDescHeap = nullptr;
	basicDescHeapDesc = {};
	basicDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	basicDescHeapDesc.NumDescriptors = 1 + pmdMaterialInfo->materialNum * 5; // �s��cbv,material cbv + �e�N�X�`��srv, sph,spa,toon
	basicDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	basicDescHeapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap
	(
		&basicDescHeapDesc,
		IID_PPV_ARGS(basicDescHeap.GetAddressOf())
	);

	//DSV�r���[�p�Ƀf�B�X�N���v�^�q�[�v�쐬
	//ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;

	result = _dev->CreateDescriptorHeap
	(
		&dsvHeapDesc,
		IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())
	);

	// ����������8�F�e�r���[���쐬

	vbView = {};
	vbView.BufferLocation = bufferHeapCreator->GetVertBuff()->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = pmdMaterialInfo->vertices.size();//�S�o�C�g��
	vbView.StrideInBytes = pmdMaterialInfo->pmdvertex_size;//1���_������̃o�C�g��

	ibView = {};
	ibView.BufferLocation = bufferHeapCreator->GetIdxBuff()->GetGPUVirtualAddress();
	ibView.SizeInBytes = sizeof(pmdMaterialInfo->indices[0]) * pmdMaterialInfo->indices.size();
	ibView.Format = DXGI_FORMAT_R16_UINT;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {}; // �s��p
	cbvDesc.BufferLocation = matrixBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = matrixBuff->GetDesc().Width;

	D3D12_CONSTANT_BUFFER_VIEW_DESC materialCBVDesc = {}; // �}�e���A�����A�e�N�X�`���Asph
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

	//�s��pcbv,�}�e���A�����pcbv,�e�N�X�`���psrv�����Ԃɐ���
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

	// ���e�N�X�`���o�b�t�@
	whiteBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0xff);
	// ���e�N�X�`���o�b�t�@
	BlackBuff = CreateD3DX12ResourceBuffer::CreateColorTexture(_dev, 0x00);
	// �O���[�O���f�[�V����
	grayTexBuff = CreateD3DX12ResourceBuffer::CreateGrayGradationTexture(_dev);

	//�}�e���A���p��cbv,srv���쐬
	for (int i = 0; i < pmdMaterialInfo->materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialCBVDesc.BufferLocation += materialBuffSize;

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

	//// ����������9�F�t�F���X�̐���
	//	ID3D12Fence* _fence = nullptr;
	//	UINT64 _fenceVal = 0;
	//	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// ����������10�F�C�x���g�n���h���̍쐬
	// ����������11�FGPU�̏��������҂�

		//���`��
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
		if (msg.message == WM_QUIT)
		{
			break;
		}

		//�ȉ��͕s�v�B_rootSignature��_pipelineState�ɑg�ݍ��܂�Ă���ASetPipe...�ł܂Ƃ߂ăZ�b�g����Ă��邩��B
		//_cmdList->SetGraphicsRootSignature(_rootSignature);

		_cmdList->SetPipelineState(_pipelineState.Get());
		_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature().Get());
		//_cmdList->SetGraphicsRootSignature(setRootSignature->GetRootSignature());
		_cmdList->RSSetViewports(1, prepareRenderingWindow->GetViewPortPointer());
		_cmdList->RSSetScissorRects(1, prepareRenderingWindow->GetRectPointer());

		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//���݂̃o�b�N�o�b�t�@���C���f�b�N�X�ɂĎ擾

		//���\�[�X�o���A�̏���
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		//���\�[�X�o���A�F���\�[�X�ւ̕����̃A�N�Z�X�𓯊�����K�v�����邱�Ƃ��h���C�o�[�ɒʒm
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//�n���h���̏����l�A�h���X�Ƀo�b�t�@�C���f�b�N�X����Z���A�e�n���h���̐擪�A�h���X���v�Z
		handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart(); // auto rtvh��handle�ɏ㏑���ł���
		handle.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvh = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->OMSetRenderTargets(1, &handle, true, &dsvh);//�����_�[�^�[�Q�b�g�Ɛ[�x�X�e���V���� CPU �L�q�q�n���h����ݒ�
		_cmdList->ClearDepthStencilView(dsvh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // �[�x�o�b�t�@�[���N���A
		//��ʃN���A
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);

		//�v���~�e�B�u�^�Ɋւ�����ƁA���̓A�Z���u���[�X�e�[�W�̓��̓f�[�^���L�q����f�[�^�������o�C���h
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//���_�o�b�t�@�[��CPU�L�q�q�n���h����ݒ�
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		//�C���f�b�N�X�o�b�t�@�[�̃r���[��ݒ�
		_cmdList->IASetIndexBuffer(&ibView);

		//�f�B�X�N���v�^�q�[�v�ݒ肨���
		//�f�B�X�N���v�^�q�[�v�ƃ��[�g�p�����[�^�̊֘A�t��
		//�����Ń��[�g�V�O�l�`���̃e�[�u���ƃf�B�X�N���v�^���֘A�t��
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());
		_cmdList->SetGraphicsRootDescriptorTable
		(
			0, // �o�C���h�̃X���b�g�ԍ�
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		);

		//�e�L�X�g�̂悤�ɓ����ɓ�̓��^�C�vDH���Z�b�g����ƁA�O���{�ɂ���Ă͋������ω�����B
		// ��ڂ̃Z�b�g�ɂ��NS300/H�ł̓��f�����\������Ȃ��Ȃ����B
		//_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		//_cmdList->SetGraphicsRootDescriptorTable
		//(
		//	1, // �o�C���h�̃X���b�g�ԍ�
		//	basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		//);

		// �}�e���A����
		auto materialHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
		auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto materialHInc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		materialHandle.ptr += inc;
		unsigned int idxOffset = 0;

		for (auto m : pmdMaterialInfo->materials)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, materialHandle);
			//�C���f�b�N�X�t���C���X�^���X�����ꂽ�v���~�e�B�u��`��
			_cmdList->DrawIndexedInstanced(m.indiceNum, 1, idxOffset, 0, 0);

			materialHandle.ptr += materialHInc;
			idxOffset += m.indiceNum;
		}

		//_cmdList->DrawInstanced(vertNum ,1, 0, 0);

		//�ޯ��ޯ̧�\���O�Ƀ��\�[�X��COMMON��ԂɈڍs
		//�R�}���h���X�g�N���[�Y��́A�R�}���h���X�g������̌Ăяo��(Reset())�ȊO�͎󂯕t�����A�ȉ�3�s�̓G���[�ɂȂ�
		//�N���[�Y��ɃR�}���h�L���[�����s���Ă��邪�A�����Ń��\�[�X�̏�Ԃ��K�p�����B�����܂ł�COMMON�����Ԃ�
		//�ύX�����Ă����K�v�����邪�A�����́���������`�N���[�Y�܂łɕύX������K�v������B
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//�����������F�R�}���h���X�g�̃N���[�Y(�R�}���h���X�g�̎��s�O�ɂ͕K���N���[�Y����)
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
		pmdMaterialInfo->angle = 200.0f;
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