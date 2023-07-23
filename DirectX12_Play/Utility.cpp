#include <stdafx.h>
#include <Utility.h>

void Utility::EnableDebugLayer() {
	ComPtr<ID3D12Debug> debuglayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(debuglayer.ReleaseAndGetAddressOf()));

	debuglayer->EnableDebugLayer();
	debuglayer->Release();//�L������ɃC���^�[�t�F�C�X���������
}

size_t
Utility::AlignmentSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

std::string Utility::GetTexPathFromModeAndTexlPath
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

std::wstring Utility::GetWideStringFromSring(const std::string& str)
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

std::string Utility::GetExtension(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

std::pair<std::string, std::string> Utility::SplitFileName(
	const std::string& path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);

	return ret;
}

std::vector<float> Utility::GetGaussianWeight(size_t count, float disp)
{
	std::vector<float> weight(count); // �E�F�C�g�z��ԋp�p
	float x = 0.0f;
	float total = 0.0f;
	for (auto& wgt : weight)
	{
		wgt = expf(-(x * x) / (2 * disp * disp));
		total += wgt;
		x += 1.0f;
	}

	total = total * 2 - 1;

	// ������1�ɂȂ�悤�ɂ���
	for (auto& wgt : weight)
	{
		wgt /= total;
	}

	return weight;
}