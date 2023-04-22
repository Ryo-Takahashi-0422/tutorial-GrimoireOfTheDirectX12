#include <DirectXTex.h>
#pragma comment (lib, "DirectXTex.lib")
#include <Windows.h>
#include<tchar.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>//�V�F�[�_�[�R���p�C���ɕK�v
#pragma comment(lib, "d3dcompiler.lib")

#include <d3dx12.h>
#include <string.h>
#include <map>
#include <sys/stat.h>

using namespace DirectX;
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

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

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapChain = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
ID3D10Blob* _vsBlob = nullptr; // ���_�V�F�[�_�[�I�u�W�F�N�g�i�[�p
ID3D10Blob* _psBlob = nullptr; // �s�N�Z���V�F�[�_�[�I�u�W�F�N�g�i�[�p
ID3DBlob* _rootSigBlob = nullptr; // ���[�g�V�O�l�`���I�u�W�F�N�g�i�[�p
ID3DBlob* errorBlob = nullptr; // �V�F�[�_�[�֘A�G���[�i�[�p

const unsigned int window_width = 720;
const unsigned int window_height = 720;

struct Vertex //?�@����vertices�����ɐ錾����ƁAstd::copy�ŃG���[��������
{
	XMFLOAT3 pos; // xyz���W
	XMFLOAT2 uv; // uv���W
};

unsigned short indices[] =
{
	0,1,2,
	2,1,3
};

//PMD�w�b�_�[�\����
struct PMDHeader
{
	float version; // 00 00 80 3F == 1.00
	char model_name[20]; // ���f����
	char comment[256]; // �R�����g
};

//PMD ���_�\����
struct PMDVertex
{   //38byte
	float pos[3]; // x, y, z // ���W 12byte
	float normal_vec[3]; // nx, ny, nz // �@���x�N�g�� 12byte
	float uv[2]; // u, v // UV���W // MMD�͒��_UV 8byte
	unsigned short bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe�� 4byte
	unsigned char bone_weight; // �{�[��1�ɗ^����e���x // min:0 max:100 // �{�[��2�ւ̉e���x�́A(100 - bone_weight) 1byte
	unsigned char edge_flag; // 0:�ʏ�A1:�G�b�W���� // �G�b�W(�֊s)���L���̏ꍇ 1byte
};

//�V�F�[�_�[���ɓn����{�I�ȍs��f�[�^
struct SceneMatrix
{
	XMMATRIX world; // ���f���{�̂̉�]�E�ړ��s��
	XMMATRIX view; // �r���[�s��
	XMMATRIX proj; // �v���W�F�N�V�����s��
	XMFLOAT3 eye; // ���_���W
};

//�}�e���A���ǂݍ��ݗp�̍\����2�Z�b�g
struct PMDMaterialSet1
{ //46Byte�ǂݍ��ݗp
	XMFLOAT3 diffuse;
	float alpha;
	float specularity;
	XMFLOAT3 specular;
	XMFLOAT3 ambient;
	unsigned char toonIdx;
	unsigned char edgeFlg;
};
struct PMDMaterialSet2
{ //24Byte�ǂݍ��ݗp
	unsigned int indicesNum;
	char texFilePath[20];
};

//�ǂݍ��񂾃}�e���A���\���̂̏o�͗p�ɕ���
struct MaterialForHlsl // ���C��
{
	XMFLOAT3 diffuse;
	float alpha;
	XMFLOAT3 specular;
	float specularity;
	XMFLOAT3 ambient;
};
struct AdditionalMaterial // �T�u
{
	std::string texPath;
	int toonIdx;
	bool edgeFlg;
};
struct Material // �W����
{
	unsigned int indiceNum;
	MaterialForHlsl material;
	AdditionalMaterial addtional;
};

void EnableDebugLayer() {
	ID3D12Debug* debuglayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debuglayer));

	debuglayer->EnableDebugLayer();
	debuglayer->Release();//�L������ɃC���^�[�t�F�C�X���������
}

//���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
//@param modelPath �A�v������݂�pmd���f���̃p�X
//@param texPath pmd���f������݂��e�N�X�`���̃p�X
//@param return �A�v������݂��e�N�X�`���̃p�X
std::string GetTexPathFromModeAndTexlPath
(
	const std::string modelPath,
	const char* texPath
)
{
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');
	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1); // ������\���擾���邽�� +1

	return folderPath + texPath;
}
size_t AlignmentSize(size_t size, size_t alignment);
// std::string(�}���`�o�C�g������)��std::wstring(���C�h������)�𓾂�
// @param str �}���`�o�C�g������
// return �ϊ����ꂽ���C�h������
std::wstring GetWideStringFromSring(const std::string& str)
{
	//�Ăяo������(�����񐔂𓾂�)
	auto num1 = MultiByteToWideChar // �֐�������lpWideCharStr���w���o�b�t�@�ɏ������܂ꂽ���C�h�����̐����Ԃ�
	(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), // �ϊ����镶����ւ̃|�C���^���w��
		-1, // pMultiByteStr ���w��������̃T�C�Y���o�C�g�P�ʂœn���B-1��NULL�I�[�ƌ��Ȃ���A�����������I�Ɍv�Z�����
		nullptr, // �ϊ���̕�������󂯎��o�b�t�@�ւ̃|�C���^���w��
		0 // lpWideCharStr���w���o�b�t�@�T�C�Y�����C�h�������̒P�ʂŎw��B 0�ŕK�v�ȃo�b�t�@�̃T�C�Y(���C�h������)���Ԃ�
	);

	std::wstring wstr; // string��wchar_t��
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar
	(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0], // wstr[0]����num1���̃��C�h������������
		num1
	);

	assert(num1 == num2);
	return wstr;
}

//�t�@�C������g���q���擾����
// @param path �Ώۃp�X������
// @return �g���q
std::string GetExtension(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

//�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
// @param path �Ώۂ̃p�X
// @param splitter ������
// @return �����O��̕�����y�A
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter = '*')
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);

	return ret;
}

// �e�N�X�`���p�@CPU����̃A�b�v���[�h�p�o�b�t�@�AGPU����̓ǂݎ��p�o�b�t�@�ADirectX::Image����
// @param metaData ���[�h�����t�@�C����Texmetadata�I�u�W�F�N�g
// @param img ���[�h�����t�@�C����Image�I�u�W�F�N�g
// @return CPU����̃A�b�v���[�h�p�o�b�t�@,GPU����̓ǂݎ��p�o�b�t�@,DirectX::TexMetadata,DirectX::Image
std::tuple<ID3D12Resource*, ID3D12Resource*>  LoadTextureFromFile(TexMetadata* metaData, Image* img, std::string& texPath)
{
	std::map<std::string, std::tuple<ID3D12Resource*, ID3D12Resource*>> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	//���\�b�h����img�����I�������m�ۂ̃|�C���^��Ԃ����Ƃ͕s�\�Ɨ�������
	ID3D12Resource* texUploadBuff = nullptr;//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[

	//�e�N�X�`���o�b�t�@�[�p��CPU�����^�q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES texUploadHeapProp;
	texUploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	texUploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texUploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texUploadHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���
	texUploadHeapProp.VisibleNodeMask = 0; // �P��A�_�v�^�[�̂���

	//�A�b�v���[�h�p
	D3D12_RESOURCE_DESC texUploadResourceDesc = {};
	texUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texUploadResourceDesc.Alignment = 0;
	texUploadResourceDesc.Width = AlignmentSize(img->slicePitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;// *5;
	texUploadResourceDesc.Height = 1;
	texUploadResourceDesc.DepthOrArraySize = 1;
	texUploadResourceDesc.MipLevels = 1;
	texUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	texUploadResourceDesc.SampleDesc.Count = 1;
	texUploadResourceDesc.SampleDesc.Quality = 0;
	texUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//CPU����̃A�b�v���[�h�p�e�N�X�`���o�b�t�@�[���쐬
	HRESULT result = _dev->CreateCommittedResource
	(&texUploadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texUploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload�^�C�v�̃q�[�v�ɂ����鐄���ݒ�
		nullptr,
		IID_PPV_ARGS(&texUploadBuff)
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}


	ID3D12Resource* texReadBuff = nullptr;//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[

	//�e�N�X�`���o�b�t�@�[�p��GPU�����^�q�[�v�v���p�e�B�ݒ�
	D3D12_HEAP_PROPERTIES texReadHeapProp = {};
	texReadHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	texReadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texReadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texReadHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���
	texReadHeapProp.VisibleNodeMask = 0; // �P��A�_�v�^�[�̂���

	//GPU�ǂݎ��p
	D3D12_RESOURCE_DESC texReadResourceDesc = {};
	texReadResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData->dimension);
	texReadResourceDesc.Alignment = 0;
	texReadResourceDesc.Width = metaData->width;
	texReadResourceDesc.Height = metaData->height;
	texReadResourceDesc.DepthOrArraySize = metaData->arraySize;
	texReadResourceDesc.MipLevels = metaData->mipLevels;
	texReadResourceDesc.Format = metaData->format;
	texReadResourceDesc.SampleDesc.Count = 1;
	texReadResourceDesc.SampleDesc.Quality = 0;
	texReadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texReadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//GPU����̓ǂݎ��p�e�N�X�`���o�b�t�@�[���쐬
	result = _dev->CreateCommittedResource
	(&texReadHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texReadResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // �o�b�t�@�[��CPU����̃��\�[�X�R�s�[��ł��邱�Ƃ�����
		nullptr,
		IID_PPV_ARGS(&texReadBuff)
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}

	_resourceTable[texPath] = std::forward_as_tuple(texUploadBuff, texReadBuff);
	return std::forward_as_tuple(texUploadBuff, texReadBuff);
}

ID3D12Resource* CreateMappedSphSpaTexResource(TexMetadata* metaData, Image* img, std::string texPath)
{
	// FlyWeight Pattern�F����e�N�X�`������̃��\�[�X������h���A�����̃��\�[�X��Ԃ�
	std::map<std::string, ID3D12Resource*> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProps.VisibleNodeMask = 0;
	heapProps.CreationNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData->dimension);
	resDesc.Alignment = 0;
	resDesc.Width = metaData->width;
	resDesc.Height = metaData->height;
	resDesc.DepthOrArraySize = metaData->arraySize;
	resDesc.MipLevels = metaData->mipLevels;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// B8G8R8X8��srv��������ƃG���[ metaData->format;
	//resDesc.Format = metaData->format;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* MappedBuff = nullptr;
	auto result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&MappedBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = MappedBuff->WriteToSubresource
	(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = MappedBuff;
	return MappedBuff;
}

ID3D12Resource* CreateColorTexture(const int param) 
{
	ID3D12Resource* whiteBuff = nullptr;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProps.VisibleNodeMask = 0;
	heapProps.CreationNodeMask = 0;

	D3D12_RESOURCE_DESC whiteResDesc = {};
	whiteResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	whiteResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	whiteResDesc.Width = 4;
	whiteResDesc.Height = 4;
	whiteResDesc.DepthOrArraySize = 1;
	whiteResDesc.SampleDesc.Count = 1;
	whiteResDesc.SampleDesc.Quality = 0;
	whiteResDesc.MipLevels = 1;
	whiteResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	whiteResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT result = _dev->CreateCommittedResource
	(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&whiteResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), param);

	result = whiteBuff->WriteToSubresource
	(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size() // �\�[�X�f�[�^��1�̐[�x�X���C�X���玟�̃f�[�^�܂ł̋����A�܂�T�C�Y
	);

	return whiteBuff;
}

//�g�D�[���̂��߂̃O���f�[�V�����e�N�X�`��
ID3D12Resource* CreateGrayGradationTexture() {

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;//��
	resDesc.Height = 256;//����
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//���C�A�E�g�ɂ��Ă͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//�Ƃ��Ƀt���O�Ȃ�

	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	ID3D12Resource* grayBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&grayBuff)
	);
	if (FAILED(result)) {
		return nullptr;
	}

	//�オ�����ĉ��������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0x30;
	int i = 0;
	for (; it != data.end(); it += 4) {
		auto col = (0000 << 24) | RGB(c, c, c);
		std::fill(it, it + 3, col);
		i++;
		if (i == 85)
		{
			c = 0xa0;
		}

		if (i == 170)
		{
			c = 0xff;
		}
	}

	result = grayBuff->WriteToSubresource
	(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size()
	);

	return grayBuff;
}

//�A���C�����g�ɂ��낦���T�C�Y��Ԃ�
//@param size ���̃T�C�Y
//@param alignment �A���C�����g�T�C�Y
//@return �A���C�����g�𑵂����T�C�Y
size_t AlignmentSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif

	//PMD�w�b�_�t�@�C���̓ǂݍ���
	char signature[3] = {}; // �V�O�l�`��
	PMDHeader pmdHeader = {};
	std::string strModelPath = "C:\\Users\\takataka\\source\\repos\\DirectX12_Play\\model\\�����~�N.pmd";
	auto fp = fopen(strModelPath.c_str(), "rb");

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	//PMD���_���̓ǂݍ���
	unsigned int vertNum; // ���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr size_t pmdvertex_size = 38;
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	//�ʒ��_���X�g
	std::vector<unsigned short> indices;
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);

	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	//�}�e���A���ǂݍ��݂ƃV�F�[�_�[�ւ̏o�͏���
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, fp);

	std::vector<PMDMaterialSet1> pmdMat1(materialNum);
	std::vector<PMDMaterialSet2> pmdMat2(materialNum);
	std::vector<Material> materials(materialNum);

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

	//�E�B���h�E�N���X�̐����Ə�����
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)windowProcedure;
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);

	//��L�E�B���h�E�N���X�̓o�^�BWINDCLASSEX�Ƃ��Ĉ�����B
	RegisterClassEx(&w);

	RECT wrc = { 0,0,window_width, window_height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(
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

	//�E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	D3D12_RECT scissorRect = {};
	scissorRect.top = 0; //�؂蔲������W
	scissorRect.left = 0; //�؂蔲�������W
	scissorRect.right = scissorRect.left + window_width; //�؂蔲���E���W
	scissorRect.bottom = scissorRect.top + window_height; //�؂蔲�������W

//���p�C�v���C���������@�����P�`�V
// 
//�����������P�F�f�o�b�O���C���[���I����
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	//�����������Q�F�f�o�C�X�̍쐬 

			//�t�@�N�g���[�̐���
	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory))))
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory))))
		{
			return -1;
		}
	}

	//�O���{�������}������Ă���ꍇ�ɃA�_�v�^�[��I�����邽�߂̏���
	//�A�_�v�^�[�̗񋓗p
	std::vector <IDXGIAdapter*> adapters;
	//����̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		adapters.push_back(tmpAdapter);
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
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;//�����\�ȃo�[�W���������������烋�[�v���f
		}
	}

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	//�����������R�F�R�}���h�L���[�̋L�q�p�ӁE�쐬

		//�R�}���h�L���[�����A�ڍ�obj����
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g����
	cmdQueueDesc.NodeMask = 0;//�A�_�v�^�[��������g��Ȃ��Ƃ���0��OK
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�R�}���h�L���[�̗D��x
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�R�}���h���X�g�ƍ��킹��

	//�R�}���h�L���[����
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

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
		_cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapChain);

	//�����������T�F�����_�[�^�[�Q�b�g�r���[(RTV)�̋L�q�q�q�[�v���쐬
			//RTV �L�q�q�q�[�v�̈�̊m��
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	//�L�q�q�q�[�v�̐����@ID3D12DescriptorHeap�F�L�q�q�̘A�������R���N�V����
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	//�ȉ��̂悤�ɋL�q���邱�ƂŃX���b�v�`�F�[���̎�����V����Desc�I�u�W�F�N�g�ɃR�s�[�ł���
	//DXGI_SWAP_CHAIN_DESC swcDesc = {};//�X���b�v�`�F�[���̐���
	//result = _swapChain->GetDesc(&swcDesc);//SWC�̐������擾����

//�����������U�F�t���[�����\�[�X(�e�t���[���̃����_�[�^�[�Q�b�g�r���[)���쐬
	std::vector<ID3D12Resource*> _backBuffers(swapChainDesc.BufferCount);//���\�[�X�o�b�t�@�[
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();//�q�[�v�̐擪��\�� CPU �L�q�q�n���h�����擾

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int idx = 0; idx < swapChainDesc.BufferCount; idx++)
	{   //swapEffect DXGI_SWAP_EFFECT_FLIP_DISCARD �̏ꍇ�͍ŏ��̃o�b�t�@�[�̂݃A�N�Z�X�\
		result = _swapChain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));//SWC�Ƀo�b�t�@�[��IID�Ƃ���IID�|�C���^��������(SWC�������_�����O���ɃA�N�Z�X����)

		_dev->CreateRenderTargetView//���\�[�X�f�[�^(_backBuffers)�ɃA�N�Z�X���邽�߂̃����_�[�^�[�Q�b�g�r���[��handle�A�h���X�ɍ쐬
		(
			_backBuffers[idx],//�����_�[�^�[�Q�b�g��\�� ID3D12Resource �I�u�W�F�N�g�ւ̃|�C���^�[
			&rtvDesc,//�����_�[ �^�[�Q�b�g �r���[���L�q���� D3D12_RENDER_TARGET_VIEW_DESC �\���̂ւ̃|�C���^�[�B
			handle//�V�����쐬���ꂽ�����_�[�^�[�Q�b�g�r���[�����݂��鈶���\�� CPU �L�q�q�n���h��(�q�[�v��̃A�h���X)
		);

		//handle���ڽ���L�q�q�̃A�h���X�������L�q�q�T�C�Y���I�t�Z�b�g���Ă���
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//�����������V�F�R�}���h�A���P�[�^�[���쐬
			//�R�}���h�A���P�[�^�[����>>�R�}���h���X�g�쐬
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));


	//�����\�[�X������
	// ����������1�F���[�g�V�O�l�`���ݒ�

		//�T���v���[�쐬
	D3D12_STATIC_SAMPLER_DESC stSamplerDesc[2] = {};
	stSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	stSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	stSamplerDesc[0].MipLODBias = 0;
	stSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	stSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	stSamplerDesc[0].MinLOD = 0;
	stSamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	stSamplerDesc[0].ShaderRegister = 0; // ���W�X�^�[�Fs�̔ԍ�
	stSamplerDesc[0].RegisterSpace = 0;
	stSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	stSamplerDesc[1] = stSamplerDesc[0];
	stSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // �J��Ԃ��Ȃ�
	stSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	stSamplerDesc[1].ShaderRegister = 1;

	//�T���v���[�̃X���b�g�ݒ�
	D3D12_DESCRIPTOR_RANGE descTableRange[3] = {};
	descTableRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // �s��p
	descTableRange[0].NumDescriptors = 1; // �g��DH��1��
	descTableRange[0].BaseShaderRegister = 0; // CBV���W�X�^0��
	descTableRange[0].RegisterSpace = 0;
	descTableRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTableRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // �}�e���A���r���[�p
	descTableRange[1].NumDescriptors = 1; // �}�e���A��CBV��1��
	descTableRange[1].BaseShaderRegister = 1; // CBV���W�X�^1��
	descTableRange[1].RegisterSpace = 0;
	descTableRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTableRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // �V�F�[�_�[���\�[�X�r���[�p
	descTableRange[2].NumDescriptors = 4; // �e�N�X�`��(�ʏ�ƃX�t�B�A�t�@�C����2��
	descTableRange[2].BaseShaderRegister = 0; // SRV���W�X�^0��
	descTableRange[2].RegisterSpace = 0;
	descTableRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

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
		&_rootSigBlob,
		&errorBlob
	);

	ID3D12RootSignature* _rootSignature = nullptr;

	result = _dev->CreateRootSignature
	(
		0,
		_rootSigBlob->GetBufferPointer(),
		_rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature)
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
		&_vsBlob
		, &errorBlob
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
		&_psBlob
		, &errorBlob
	);

	//�G���[�`�F�b�N
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C����������܂���");
			return 0;
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
	gpipeLine.pRootSignature = _rootSignature;

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

	ID3D12PipelineState* _pipelineState = nullptr;

	result = _dev->CreateGraphicsPipelineState(&gpipeLine, IID_PPV_ARGS(&_pipelineState));


	// ����������5�F�R�}���h���X�g����

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

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

	ID3D12Resource* vertBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	ID3D12Resource* idxBuff = nullptr;//CPU��GPU�̋��L?�o�b�t�@�[�̈�(���\�[�X�ƃq�[�v)
	std::vector<ID3D12Resource*> texUploadBuff(materialNum);//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[
	std::vector<ID3D12Resource*> texReadBuff(materialNum);//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[
	std::vector<ID3D12Resource*> sphMappedBuff(materialNum);//sph�p�o�b�t�@�[
	std::vector<ID3D12Resource*> spaMappedBuff(materialNum);//spa�p�o�b�t�@�[
	std::vector<ID3D12Resource*> toonUploadBuff(materialNum);//�g�D�[���p�A�b�v���[�h�o�b�t�@�[
	std::vector<ID3D12Resource*> toonReadBuff(materialNum);//�g�D�[���p���[�h�o�b�t�@�[
	ID3D12Resource* matrixBuff = nullptr; // �s��p�萔�o�b�t�@�[
	ID3D12Resource* materialBuff = nullptr; // �}�e���A���p�萔�o�b�t�@�[
	ID3D12Resource* depthBuff = nullptr; // �f�v�X�o�b�t�@�[

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
		IID_PPV_ARGS(&vertBuff)
	);

	//�C���f�b�N�X�o�b�t�@�[���쐬(���\�[�X�ƈÖٓI�ȃq�[�v�̍쐬)
	result = _dev->CreateCommittedResource
	(&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&indicesDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff)
	);

	//�f�v�X�o�b�t�@�[���쐬
	result = _dev->CreateCommittedResource
	(
		&depthHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		nullptr,
		IID_PPV_ARGS(&depthBuff)
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

	std::vector<DirectX::TexMetadata*> metaData(materialNum);
	std::vector<DirectX::Image*> img(materialNum);
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
			auto namePair = SplitFileName(texFileName);

			if (GetExtension(namePair.first) == "sph" || GetExtension(namePair.first) == "spa")
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

		auto texFilePath = GetTexPathFromModeAndTexlPath(strModelPath, texFileName.c_str());
		auto wTexPath = GetWideStringFromSring(texFilePath);
		auto extention = GetExtension(texFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			return 0;
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
		
		if (GetExtension(texFileName) == "sph")
		{
			sphMappedBuff[i] = CreateMappedSphSpaTexResource(metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			spaMappedBuff[i] = nullptr;
		}

		else if (GetExtension(texFileName) == "spa")
		{
			spaMappedBuff[i] = CreateMappedSphSpaTexResource(metaData[i], img[i], texFilePath);
			std::tie(texUploadBuff[i], texReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
			sphMappedBuff[i] = nullptr;
		}

		else
		{
			std::tie(texUploadBuff[i], texReadBuff[i]) = LoadTextureFromFile(metaData[i], img[i], texFilePath);
			sphMappedBuff[i] = nullptr;
			spaMappedBuff[i] = nullptr;	
		}
	}

	// �g�D�[������
	std::string toonFilePath = "toon\\";
	struct _stat s = {};
	std::vector<DirectX::TexMetadata*> toonMetaData(materialNum);
	std::vector<DirectX::Image*> toonImg(materialNum);
	ScratchImage toonScratchImg = {};

	for (int i = 0; i < materials.size(); i++) 
	{
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFileName[16];
		sprintf(toonFileName, "toon%02d.bmp", materials[i].addtional.toonIdx + 1);
		toonFilePath += toonFileName;
		toonFilePath = GetTexPathFromModeAndTexlPath(strModelPath, toonFilePath.c_str());

		auto wTexPath = GetWideStringFromSring(toonFilePath);
		auto extention = GetExtension(toonFilePath);

		if (!loadLambdaTable.count(extention))
		{
			std::cout << "�ǂݍ��߂Ȃ��e�N�X�`�������݂��܂�" << std::endl;
			return 0;
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
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = LoadTextureFromFile(toonMetaData[i], toonImg[i], toonFilePath);
		}

		else
			std::tie(toonUploadBuff[i], toonReadBuff[i]) = std::forward_as_tuple(nullptr, nullptr);
	}

	//�s��p�萔�o�b�t�@�[�̐���
	XMMATRIX worldMat = XMMatrixIdentity();
	//auto worldMat = XMMatrixRotationY(15.0f);
	float angle = 0.0f;

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
		IID_PPV_ARGS(&matrixBuff)
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
		IID_PPV_ARGS(&materialBuff)
	);

	//���_�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɒ��_�f�[�^���R�s�[����B
	//CPU�͈ÖٓI�ȃq�[�v�̏��𓾂��Ȃ����߁AMap�֐��ɂ��VRAM��̃o�b�t�@�[�ɃA�h���X�����蓖�Ă���Ԃ�
	//���_�Ȃǂ̏���VRAM�փR�s�[���Ă���(���̂R��CPU��GPU�ǂ�����A�N�Z�X�\��UPLOAD�^�C�v�ȃq�[�v�̃}�b�v�\)�A
	//s�Ƃ��������BUnmap�̓R�����g�A�E�g���Ă����ɉe���͂Ȃ���...
	unsigned char* vertMap = nullptr;

	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	//�C���f�N�X�o�b�t�@�[�̉��z�A�h���X���|�C���^�Ƀ}�b�v(�֘A�t��)���āA���z�I�ɃC���f�b�N�X�f�[�^���R�s�[����B
	unsigned short* mappedIdx = nullptr;
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//�s��p�萔�o�b�t�@�[�̃}�b�s���O
	SceneMatrix* mapMatrix = nullptr;
	result = matrixBuff->Map(0, nullptr, (void**)&mapMatrix);
	mapMatrix->world = worldMat;
	mapMatrix->view = viewMat;
	mapMatrix->proj = projMat;
	mapMatrix->eye = eye;

	//�}�e���A���p�o�b�t�@�[�ւ̃}�b�s���O
	char* mapMaterial = nullptr;
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
		auto rowPitch = AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
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
		auto toonrowPitch = AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
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

		src[matNum].pResource = texUploadBuff[matNum];
		src[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src[matNum].PlacedFootprint.Offset = 0;
		src[matNum].PlacedFootprint.Footprint.Width = metaData[matNum]->width;
		src[matNum].PlacedFootprint.Footprint.Height = metaData[matNum]->height;
		src[matNum].PlacedFootprint.Footprint.Depth = metaData[matNum]->depth;
		src[matNum].PlacedFootprint.Footprint.RowPitch =
			AlignmentSize(img[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		src[matNum].PlacedFootprint.Footprint.Format = img[matNum]->format;//metaData.format;

		//�R�s�[��ݒ�
		dst[matNum].pResource = texReadBuff[matNum];
		dst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&dst[matNum], 0, 0, 0, &src[matNum], nullptr);

			//�o���A�ݒ�
			texBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			texBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			texBarriierDesc[matNum].Transition.pResource = texReadBuff[matNum];
			texBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			texBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			texBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &texBarriierDesc[matNum]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			_cmdQueue->Signal(_fence, ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator, nullptr);
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

		toonSrc[matNum].pResource = toonUploadBuff[matNum];
		toonSrc[matNum].Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		toonSrc[matNum].PlacedFootprint.Offset = 0;
		toonSrc[matNum].PlacedFootprint.Footprint.Width = toonMetaData[matNum]->width;
		toonSrc[matNum].PlacedFootprint.Footprint.Height = toonMetaData[matNum]->height;
		toonSrc[matNum].PlacedFootprint.Footprint.Depth = toonMetaData[matNum]->depth;
		toonSrc[matNum].PlacedFootprint.Footprint.RowPitch =
			AlignmentSize(toonImg[matNum]->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT); // R8G8B8A8:4bit * width�̒l��256�̔{���ł��邱��
		toonSrc[matNum].PlacedFootprint.Footprint.Format = toonImg[matNum]->format;

		//�R�s�[��ݒ�
		toonDst[matNum].pResource = toonReadBuff[matNum];
		toonDst[matNum].Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		toonDst[matNum].SubresourceIndex = 0;

		{
			_cmdList->CopyTextureRegion(&toonDst[matNum], 0, 0, 0, &toonSrc[matNum], nullptr);

			//�o���A�ݒ�
			toonBarriierDesc[matNum].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			toonBarriierDesc[matNum].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			toonBarriierDesc[matNum].Transition.pResource = toonReadBuff[matNum];
			toonBarriierDesc[matNum].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			toonBarriierDesc[matNum].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			toonBarriierDesc[matNum].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			_cmdList->ResourceBarrier(1, &toonBarriierDesc[matNum]);
			_cmdList->Close();
			//�R�}���h���X�g�̎��s
			ID3D12CommandList* cmdlists[] = { _cmdList };
			_cmdQueue->ExecuteCommandLists(1, cmdlists);
			////�҂�
			_cmdQueue->Signal(_fence, ++_fenceVal);

			if (_fence->GetCompletedValue() != _fenceVal) {
				auto event = CreateEvent(nullptr, false, false, nullptr);
				_fence->SetEventOnCompletion(_fenceVal, event);
				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);
			}
			_cmdAllocator->Reset();//�L���[���N���A
			_cmdList->Reset(_cmdAllocator, nullptr);
		}
	}

	//�s��CBV,SRV�f�B�X�N���v�^�q�[�v�쐬
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC basicDescHeapDesc = {};
	basicDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	basicDescHeapDesc.NumDescriptors = 1 + materialNum * 5; // �s��cbv,material cbv + �e�N�X�`��srv, sph,spa,toon
	basicDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	basicDescHeapDesc.NodeMask = 0;

	result = _dev->CreateDescriptorHeap
	(
		&basicDescHeapDesc,
		IID_PPV_ARGS(&basicDescHeap)
	);

	//DSV�r���[�p�Ƀf�B�X�N���v�^�q�[�v�쐬
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;

	result = _dev->CreateDescriptorHeap
	(
		&dsvHeapDesc,
		IID_PPV_ARGS(&dsvHeap)
	);

	// ����������8�F�e�r���[���쐬

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size();//�S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	D3D12_INDEX_BUFFER_VIEW ibView = {};
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
		depthBuff,
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
	ID3D12Resource* whiteBuff = nullptr;
	whiteBuff = CreateColorTexture(0xff);
	// ���e�N�X�`���o�b�t�@
	ID3D12Resource* BlackBuff = nullptr;
	BlackBuff = CreateColorTexture(0x00);
	// �O���[�O���f�[�V����
	ID3D12Resource* grayTexBuff = nullptr;
	grayTexBuff = CreateGrayGradationTexture();

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
			(whiteBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = texReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(texReadBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// sph�t�@�C��
		if (sphMappedBuff[i] == nullptr)
		{
			srvDesc.Format = whiteBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(whiteBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = sphMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(sphMappedBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// spa�t�@�C��
		if (spaMappedBuff[i] == nullptr)
		{
			srvDesc.Format = BlackBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(BlackBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = spaMappedBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(spaMappedBuff[i], &srvDesc, basicDescHeapHandle);
		}

		basicDescHeapHandle.ptr += inc;

		// �g�D�[���e�N�X�`���t�@�C��
		if (toonReadBuff[i] == nullptr)
		{
			srvDesc.Format = grayTexBuff->GetDesc().Format;
			_dev->CreateShaderResourceView
			(grayTexBuff, &srvDesc, basicDescHeapHandle);
		}

		else
		{
			srvDesc.Format = toonReadBuff[i]->GetDesc().Format;
			_dev->CreateShaderResourceView
			(toonReadBuff[i], &srvDesc, basicDescHeapHandle);
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

		_cmdList->SetPipelineState(_pipelineState);
		_cmdList->SetGraphicsRootSignature(_rootSignature);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorRect);

		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();//���݂̃o�b�N�o�b�t�@���C���f�b�N�X�ɂĎ擾

		//���\�[�X�o���A�̏���
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
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
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
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
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//ID3D12Fence��Signal��CPU���̃t�F���X�ő������s
		//ID3D12CommandQueue��Signal��GPU���̃t�F���X��
		//�R�}���h�L���[�ɑ΂��鑼�̂��ׂĂ̑��삪����������Ƀt�F���X�X�V
		_cmdQueue->Signal(_fence, ++_fenceVal);

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
		_cmdList->Reset(_cmdAllocator, nullptr);//�R�}���h���X�g���A�V�����R�}���h���X�g���쐬���ꂽ���̂悤�ɏ�����ԂɃ��Z�b�g

		//�s����̍X�V
		angle += 0.01f;
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;


		//�t���b�v���ă����_�����O���ꂽ�C���[�W�����[�U�[�ɕ\��
		_swapChain->Present(1, 0);
	}

	UnregisterClass(w.lpszClassName, w.hInstance);
}




