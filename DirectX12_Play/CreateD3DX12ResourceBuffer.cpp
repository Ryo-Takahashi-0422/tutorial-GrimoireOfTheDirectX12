#include <stdafx.h>
#include <CreateD3DX12ResourceBuffer.h>



// �e�N�X�`���p�@CPU����̃A�b�v���[�h�p�o�b�t�@�AGPU����̓ǂݎ��p�o�b�t�@�ADirectX::Image����
// @param metaData ���[�h�����t�@�C����Texmetadata�I�u�W�F�N�g
// @param img ���[�h�����t�@�C����Image�I�u�W�F�N�g
// @return CPU����̃A�b�v���[�h�p�o�b�t�@,GPU����̓ǂݎ��p�o�b�t�@,DirectX::TexMetadata,DirectX::Image

std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>> 
CreateD3DX12ResourceBuffer::LoadTextureFromFile(ComPtr<ID3D12Device> _dev, TexMetadata* metaData, Image* img, std::string& texPath)
{
	auto& utility = Utility::Instance(); ////////////////////

	std::map<std::string, std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>>> _resourceTable;
	auto iterator = _resourceTable.find(texPath);
	if (iterator != _resourceTable.end()) {
		return iterator->second;
	};

	//���\�b�h����img�����I�������m�ۂ̃|�C���^��Ԃ����Ƃ͕s�\�Ɨ�������
	ComPtr<ID3D12Resource> texUploadBuff = nullptr;//�e�N�X�`��CPU�A�b�v���[�h�p�o�b�t�@�[

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
	texUploadResourceDesc.Width = utility.AlignmentSize(img->slicePitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;// *5;
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
		IID_PPV_ARGS(texUploadBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}


	ComPtr<ID3D12Resource> texReadBuff = nullptr;//�e�N�X�`��GPU�ǂݎ��p�o�b�t�@�[

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
		IID_PPV_ARGS(texReadBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		return std::forward_as_tuple(nullptr, nullptr);
	}

	_resourceTable[texPath] = std::forward_as_tuple(texUploadBuff.Get(), texReadBuff.Get());
	return std::forward_as_tuple(texUploadBuff.Get(), texReadBuff.Get());
}