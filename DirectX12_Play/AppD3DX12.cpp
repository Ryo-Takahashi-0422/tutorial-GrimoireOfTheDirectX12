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

///�㏈��
void AppD3DX12::Terminate()
{

};

AppD3DX12::~AppD3DX12()
{

};

//window�����b�Z�[�W���[�v���Ɏ擾�������b�Z�[�W����������N���X
LRESULT windowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);//OS�փA�v���I���̒ʒm
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

HRESULT AppD3DX12::ReadPMDHeaderFile() 
{
	// PMD�w�b�_�t�@�C���̓ǂݍ���
	auto fp = fopen(strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	// �V�O�l�`�����ǂݍ���
	fread(signature, sizeof(signature), 1, fp);

	// pmd�w�b�_�[���ǂݍ���
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	// pmd���_���̓ǂݍ���
	fread(&vertNum, sizeof(vertNum), 1, fp);

	// ���_���̃T�C�Y��38�Œ�
	pmdvertex_size = 38;

	// ���_�R���e�i�̃T�C�Y�ύX�A���_���Qt_vertex��pmd�f�[�^����ǂݍ���
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	// pmd�t�@�C���̖ʒ��_���X�g���璸�_���擾�A�R���e�i�T�C�Y�ύX�A���_�ԍ��擾
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);
	printf("%d",sizeof(indices[0]));

	// �}�e���A���ǂݍ��݂ƃV�F�[�_�[�ւ̏o�͏���
	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMat1.resize(materialNum);
	pmdMat2.resize(materialNum);
	materials.resize(materialNum);

	for (int i = 0; i < materialNum; i++)
	{
		fread(&pmdMat1[i], 46, 1, fp);
		fread(&pmdMat2[i], sizeof(PMDMaterialSet2), 1, fp);
	}

	for (int i = 0; i < materialNum; i++)
	{
		materials[i].indiceNum = pmdMat2[i].indicesNum;
		materials[i].material.diffuse = pmdMat1[i].diffuse;
		materials[i].material.alpha = pmdMat1[i].alpha;
		materials[i].material.specular = pmdMat1[i].specular;
		materials[i].material.specularity = pmdMat1[i].specularity;
		materials[i].material.ambient = pmdMat1[i].ambient;
		materials[i].addtional.texPath = pmdMat2[i].texFilePath;
	}

	fclose(fp);
	return S_OK;
};

void AppD3DX12::CreateAppWindow()
{
	//�E�B���h�E�N���X�̐����Ə�����
	w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)windowProcedure;
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);

	//��L�E�B���h�E�N���X�̓o�^�BWINDCLASSEX�Ƃ��Ĉ�����B
	RegisterClassEx(&w);

	RECT wrc = { 0,0,window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(
		w.lpszClassName,
		_T("DX12�e�X�g"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^
}

void AppD3DX12::SetViewportAndRect()
{
	viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	scissorRect = {};
	scissorRect.top = 0; //�؂蔲������W
	scissorRect.left = 0; //�؂蔲�������W
	scissorRect.right = scissorRect.left + window_width; //�؂蔲���E���W
	scissorRect.bottom = scissorRect.top + window_height; //�؂蔲�������W
}

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
	if (FAILED(AppD3DX12::ReadPMDHeaderFile())) return false;

	// �����_�����O�E�B���h�E�ݒ�
	AppD3DX12::CreateAppWindow();

	// �����_�����O�E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	// �r���[�|�[�g�ƃV�U�[�̈�̐ݒ�
	SetViewportAndRect();
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
	swapChainDesc.Width = window_width;
	swapChainDesc.Height = window_height;
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
		hwnd,
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

	return TRUE;
}

bool AppD3DX12::ResourceInit() {
	//�����\�[�X������
	// ����������1�F���[�g�V�O�l�`���ݒ�

	//�T���v���[�쐬
	CD3DX12_STATIC_SAMPLER_DESC stSamplerDesc[2] = {};
	stSamplerDesc[0].Init(0);
	stSamplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	//�T���v���[�̃X���b�g�ݒ�
	CD3DX12_DESCRIPTOR_RANGE descTableRange[3] = {};

	descTableRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // martix
	descTableRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // material
	descTableRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // colortex, graytex, spa, sph

	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1; // �f�v�X�p
	rootParam[0].DescriptorTable.pDescriptorRanges = descTableRange;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2; // �}�e���A���ƃe�N�X�`���Ŏg��
	rootParam[1].DescriptorTable.pDescriptorRanges = &descTableRange[1];
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 2;
	rootSignatureDesc.pParameters = rootParam;
	rootSignatureDesc.NumStaticSamplers = 2;
	rootSignatureDesc.pStaticSamplers = stSamplerDesc;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	result = D3D12SerializeRootSignature //�V���A����
	(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		_rootSigBlob.ReleaseAndGetAddressOf(),
		errorBlob.GetAddressOf()
	);

	_rootSignature = nullptr;

	result = _dev->CreateRootSignature
	(
		0,
		_rootSigBlob->GetBufferPointer(),
		_rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(_rootSignature.ReleaseAndGetAddressOf())
	);
	_rootSigBlob->Release();

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
		, errorBlob.GetAddressOf()
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
		, errorBlob.GetAddressOf()
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
			errstr.resize(errorBlob->GetBufferSize());

			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
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
			DXGI_FORMAT_R16G16_UINT,
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
	gpipeLine.pRootSignature = _rootSignature.Get();

	gpipeLine.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeLine.VS.BytecodeLength = _vsBlob->GetBufferSize();

	gpipeLine.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeLine.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeLine.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeLine.RasterizerState.MultisampleEnable = false;
	gpipeLine.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeLine.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeLine.RasterizerState.DepthClipEnable = true;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetdDesc = {};
	renderTargetdDesc.BlendEnable = false;//�u�����h��L���ɂ��邩�����ɂ��邩
	renderTargetdDesc.LogicOpEnable = false;//�_�������L���ɂ��邩�����ɂ��邩
	renderTargetdDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeLine.BlendState.AlphaToCoverageEnable = false;
	gpipeLine.BlendState.IndependentBlendEnable = false;
	gpipeLine.BlendState.RenderTarget[0] = renderTargetdDesc;
	gpipeLine.InputLayout.pInputElementDescs = inputLayout;

	gpipeLine.InputLayout.NumElements = _countof(inputLayout);

	gpipeLine.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipeLine.NumRenderTargets = 1;

	gpipeLine.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	gpipeLine.SampleDesc.Count = 1; //1�T���v��/�s�N�Z��
	gpipeLine.SampleDesc.Quality = 0;

	gpipeLine.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeLine.DepthStencilState.DepthEnable = true;
	gpipeLine.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �[�x�o�b�t�@�[�ɐ[�x�l��`������
	gpipeLine.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �\�[�X�f�[�^���R�s�[��f�[�^��菬�����ꍇ��������
	gpipeLine.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	_pipelineState = nullptr;

	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));


	// ����������5�F�R�}���h���X�g����

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	// ����������6�F�R�}���h���X�g�̃N���[�Y(�R�}���h���X�g�̎��s�O�ɂ͕K���N���[�Y����)
	//cmdList->Close();

	// ����������7�F�e�o�b�t�@�[���쐬���Ē��_����ǂݍ���

	//���_�o�b�t�@�[�ƃC���f�b�N�X�o�b�t�@�[�p�̃q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 0;
	heapProps.VisibleNodeMask = 0;

	//�[�x�o�b�t�@�[�p�q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES depthHeapProps = {};
	depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//�[�x�o�b�t�@�[�p���\�[�X�f�B�X�N���v�^
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l�������ݗp
	depthResDesc.SampleDesc.Count = 1; // 1pixce/1�̃T���v��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//�N���A�o�����[(����̃��\�[�X�̃N���A������œK�����邽�߂Ɏg�p�����l)
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f; // �[��1.0(�ő�l)�ŃN���A

	texUploadBuff.resize(materialNum);//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	texReadBuff.resize(materialNum);//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	sphMappedBuff.resize(materialNum);//sph�p�o�b�t�@�[
	spaMappedBuff.resize(materialNum);//spa�p�o�b�t�@�[
	toonUploadBuff.resize(materialNum);//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	toonReadBuff.resize(materialNum);//�g�D�[���p���[�h�o�b�t�@�[

	//���_�o�b�t�@�[�̍쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬) ID3D12Resource�I�u�W�F�N�g�̓����p�����[�^�ݒ�
	D3D12_RESOURCE_DESC vertresDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	D3D12_RESOURCE_DESC indicesDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices[0]) * indices.size());
	result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // ���\�[�X�̏�ԁBGPU���炵�ēǂݎ��p
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf())
	);

	//�C���f�b�N�X�o�b�t�@�[���쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬)
	result = _dev->CreateCommittedResource
	(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf())
	);

	//�f�v�X�o�b�t�@�[���쐬
	result = _dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(depthBuff.ReleaseAndGetAddressOf())
	);

	//�t�@�C���`�����̃e�N�X�`�����[�h����
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};

	metaData.resize(materialNum);
	img.resize(materialNum);
	ScratchImage scratchImg = {};
	result = CoInitializeEx(0, COINIT_MULTITHREADED);

	// �e�N�X�`���p��CPU_Upload�p�AGPU_Read�p�o�b�t�@�̍쐬
	for (int i = 0; i < materials.size(); i++)
	{
		if (strlen(materials[i].addtional.texPath.c_str()) == 0)
		{
			texUploadBuff[i] = nullptr;
			texReadBuff[i] = nullptr;
			continue;
		}

		std::string texFileName = materials[i].addtional.texPath;

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
		// �X�ɏڍׂ͕s����������ɂ��Ȃ���vertBuff�̃}�b�s���O�����s����悤�ɂȂ邽�߁A�ꎞ�������

		auto texFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, texFileName.c_str());
		auto wTexPath = Utility::GetWideStringFromSring(texFilePath);
		auto extention = Utility::GetExtension(texFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			//return 0;
			return false;
		}
		metaData[i] = new TexMetadata;
		result = loadLambdaTable[extention](wTexPath, metaData[i], scratchImg);

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

	// �g�D�[������
	std::string toonFilePath = "toon\\";
	struct _stat s = {};
	toonMetaData.resize(materialNum);
	toonImg.resize(materialNum);
	ScratchImage toonScratchImg = {};

	for (int i = 0; i < materials.size(); i++)
	{
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFileName[16];
		sprintf(toonFileName, "toon%02d.bmp", materials[i].addtional.toonIdx + 1);
		toonFilePath += toonFileName;
		toonFilePath = Utility::GetTexPathFromModeAndTexlPath(strModelPath, toonFilePath.c_str());

		auto wTexPath = Utility::GetWideStringFromSring(toonFilePath);
		auto extention = Utility::GetExtension(toonFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			//return 0;
			break;
		}

		toonMetaData[i] = new TexMetadata;
		result = loadLambdaTable[extention](wTexPath, toonMetaData[i], toonScratchImg);

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

	//�s��p�萔�o�b�t�@�[�̐���
	worldMat = XMMatrixIdentity();
	//auto worldMat = XMMatrixRotationY(15.0f);
	angle = 0.0f;

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
		static_cast<float>(window_height) / static_cast<float>(window_width),
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
	materialBuffResDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum);

	_dev->CreateCommittedResource
	(
		&materialHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&materialBuffResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
	);

	//���_�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɒ��_�f�[�^���R�s�[����B
	//CPU�͈ÖٓI�ȃq�[�v�̏��𓾂��Ȃ����߁AMap�֐��ɂ��VRAM��̃o�b�t�@�[�ɃA�h���X�����蓖�Ă���Ԃ�
	//���_�Ȃǂ̏���VRAM�փR�s�[���Ă���(���̂R��CPU��GPU�ǂ�����A�N�Z�X�\��UPLOAD�^�C�v�ȃq�[�v�̃}�b�v�\)�A
	//s�Ƃ��������BUnmap�̓R�����g�A�E�g���Ă����ɉe���͂Ȃ���...
	//vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	//�C���f�N�X�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	//mappedIdx = nullptr;
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//�s��p�萔�o�b�t�@�[�̃}�b�s���O
	//mapMatrix = nullptr;
	result = matrixBuff->Map(0, nullptr, (void**)&mapMatrix);
	mapMatrix->world = worldMat;
	mapMatrix->view = viewMat;
	mapMatrix->proj = projMat;
	mapMatrix->eye = eye;

	//�}�e���A���p�o�b�t�@�[�ւ̃}�b�s���O
	//mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	for (auto m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}
	materialBuff->Unmap(0, nullptr);

	// �e�N�X�`���A�b�v���[�h�p�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA
	// ���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	// �e�N�X�`���̃A�b�v���[�h�p�o�b�t�@�ւ̃}�b�s���O
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (texUploadBuff[matNum] == nullptr) continue;

		auto srcAddress = img[matNum]->pixels;
		auto rowPitch = Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);//////////////////////////////
		uint8_t* mapforImg = nullptr; // �s�N�Z���f�[�^�̃V�X�e���������o�b�t�@�[�ւ̃|�C���^�[��uint8_t(img->pixcel)
		result = texUploadBuff[matNum]->Map(0, nullptr, (void**)&mapforImg);

		// img:���f�[�^�̏����A�h���X(srcAddress)�����s�b�`���I�t�Z�b�g���Ȃ���A�␳�����s�b�`��(rowPitch)�̃A�h���X��
		// mapforImg�ɂ��̐���(rowPitch)�I�t�Z�b�g���J��Ԃ��R�s�[���Ă���
		//std::copy_n(img->pixels, img->slicePitch, mapforImg);
		for (int i = 0; i < img[matNum]->height; ++i)
		{
			std::copy_n(srcAddress, rowPitch, mapforImg);
			srcAddress += img[matNum]->rowPitch;
			mapforImg += rowPitch;
		}

		texUploadBuff[matNum]->Unmap(0, nullptr);
	}

	// �g�D�[���e�N�X�`�������l�Ƀ}�b�s���O
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (toonUploadBuff[matNum] == nullptr) continue;

		auto toonSrcAddress = toonImg[matNum]->pixels;
		auto toonrowPitch = Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint8_t* toonmapforImg = nullptr;
		result = toonUploadBuff[matNum]->Map(0, nullptr, (void**)&toonmapforImg);

		for (int i = 0; i < toonImg[matNum]->height; ++i)
		{
			std::copy_n(toonSrcAddress, toonrowPitch, toonmapforImg);
			toonSrcAddress += toonImg[matNum]->rowPitch;
			toonmapforImg += toonrowPitch;
		}

		toonUploadBuff[matNum]->Unmap(0, nullptr);
	}

	// �e�N�X�`���p�]���I�u�W�F�N�g
	std::vector<D3D12_TEXTURE_COPY_LOCATION> src(materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> dst(materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> texBarriierDesc(materialNum);

	// �e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (texUploadBuff[matNum] == nullptr || texReadBuff[matNum] == nullptr) continue;

		src[matNum].pResource = texUploadBuff[matNum].Get();
		src[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src[matNum].PlacedFootprint.Offset = 0;
		src[matNum].PlacedFootprint.Footprint.Width = metaData[matNum]->width;
		src[matNum].PlacedFootprint.Footprint.Height = metaData[matNum]->height;
		src[matNum].PlacedFootprint.Footprint.Depth = metaData[matNum]->depth;
		src[matNum].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//�R�s�[��ݒ�
		dst[matNum].pResource = texReadBuff[matNum].Get();
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//�o���A�ݒ�
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = texReadBuff[matNum].Get();
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
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonSrc(materialNum);
	std::vector<D3D12_TEXTURE_COPY_LOCATION> toonDst(materialNum);
	std::vector<D3D12_RESOURCE_BARRIER> toonBarriierDesc(materialNum);
	// �g�D�[���e�N�X�`����GPU��Upload�p�o�b�t�@����GPU��Read�p�o�b�t�@�փf�[�^�R�s�[
	for (int matNum = 0; matNum < materialNum; matNum++)
	{
		if (toonUploadBuff[matNum] == nullptr || toonReadBuff[matNum] == nullptr) continue;

		toonSrc[matNum].pResource = toonUploadBuff[matNum].Get();
		toonSrc[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		toonSrc[matNum].PlacedFootprint.Offset = 0;
		toonSrc[matNum].PlacedFootprint.Footprint.Width = toonMetaData[matNum]->width;
		toonSrc[matNum].PlacedFootprint.Footprint.Height = toonMetaData[matNum]->height;
		toonSrc[matNum].PlacedFootprint.Footprint.Depth = toonMetaData[matNum]->depth;
		toonSrc[matNum].PlacedFootprint.Footprint.RowPitch =
			Utility::AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//�R�s�[��ݒ�
		toonDst[matNum].pResource = toonReadBuff[matNum].Get();
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//�o���A�ݒ�
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = toonReadBuff[matNum].Get();
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
	basicDescHeapDesc.NumDescriptors = 1 + materialNum * 5; // �s��cbv,material cbv + �e�N�X�`��srv, sph,spa,toon
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
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size();//�S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.SizeInBytes = sizeof(indices[0]) * indices.size();
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
		depthBuff.Get(),
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
	for (int i = 0; i < materialNum; i++)
	{
		_dev->CreateConstantBufferView(&materialCBVDesc, basicDescHeapHandle);
		basicDescHeapHandle.ptr += inc;
		materialCBVDesc.BufferLocation += materialBuffSize;

		// �e�N�X�`��
		if (texReadBuff[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = texReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(texReadBuff[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// sph�t�@�C��
		if (sphMappedBuff[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = sphMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(sphMappedBuff[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// spa�t�@�C��
		if (spaMappedBuff[i] == nullptr)
		{
			srvDesc.Format = BlackBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(BlackBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = spaMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(spaMappedBuff[i].Get(), &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// �g�D�[���e�N�X�`���t�@�C��
		if (toonReadBuff[i] == nullptr)
		{
			srvDesc.Format = grayTexBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(grayTexBuff.Get(), &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = toonReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(toonReadBuff[i].Get(), &srvDesc, basicDescHeapHandle);
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
		_cmdList->SetGraphicsRootSignature(_rootSignature.Get());
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorRect);

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

		for (auto m : materials)
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
		angle += 0.01f;
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;


		//�t���b�v���ă����_�����O���ꂽ�C���[�W�����[�U�[�ɕ\��
		_swapChain->Present(1, 0);
	}

	delete vertMap;
	delete mappedIdx;
	delete mapMaterial;
	UnregisterClass(w.lpszClassName, w.hInstance);
}