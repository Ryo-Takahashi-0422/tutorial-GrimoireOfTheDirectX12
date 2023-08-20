#pragma once

struct PostSetting
{
	bool isFoV;
	float bloomCol[3];
	bool isSSAO;
	float dummy; // to alignment
	bool isBloom;
};

class SettingImgui
{
private:
	ComPtr<ID3D12DescriptorHeap> imguiSRVHeap = nullptr; // imgui��SRV�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> imguiRTVHeap = nullptr; // imgui��RTV�p�f�B�X�N���v�^�q�[�v
	ComPtr<ID3D12Resource> imguiBuff = nullptr; // imgui�p�o�b�t�@�[

	HRESULT CreateSRVDHeap4Imgui(ComPtr<ID3D12Device> _dev);
	HRESULT CreateRTVDHeap4Imgui(ComPtr<ID3D12Device> _dev);
	HRESULT CreateBuff4Imgui(ComPtr<ID3D12Device> _dev);
	void CreateRTV4Imgui(ComPtr<ID3D12Device> _dev);
	bool blnResult;
	float fovValueExp;
	float bgColorExp[4];
	float lightVecExp[3];
	float bloomExp[3];
	bool isFoV;
	bool isSSAO;
	bool isSelfShadowOn;
	bool isBloomOn;
	bool isEffectOn;

	PostSetting* mappedPostSetting = nullptr;

public:
	// �}���`�p�XSRV�p�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT Init(ComPtr<ID3D12Device> _dev,	PrepareRenderingWindow* pRWindow);

	void DrawDateOfImGUI(
		ComPtr<ID3D12Device> _dev,
		ComPtr<ID3D12GraphicsCommandList> _cmdList,
		ComPtr<ID3D12Resource> pResoures,
		BufferHeapCreator* bufferHeapCreator,
		UINT backBufferIndex);

	float GetFovValue() { return fovValueExp; };
	float GetBGColor(int num) { return bgColorExp[num]; };
	float GetLightVector(int num) { return lightVecExp[num]; };
	float GetBloomValue(int num) { return bloomExp[num]; };
	bool GetFoVBool() { return isFoV; };
	bool GetSSAOBool() { return isSSAO; };
	bool GetShadowmapOnOffBool() { return isSelfShadowOn; };
	bool GetBloomOnOffBool() { return isBloomOn; };
	bool GetEffectOnOffBool() { return isEffectOn; };
	size_t GetPostSettingSize() { return sizeof(PostSetting); };
};