#pragma once

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

public:
	// �}���`�p�XSRV�p�f�B�X�N���v�^�q�[�v�̍쐬
	HRESULT Init(ComPtr<ID3D12Device> _dev,	PrepareRenderingWindow* pRWindow);

	void DrawDateOfImGUI(
		ComPtr<ID3D12Device> _dev,
		ComPtr<ID3D12GraphicsCommandList> _cmdList,
		std::vector<ComPtr<ID3D12Resource>> pResoures,
		BufferHeapCreator*/*std::vector<BufferHeapCreator*>*/ bufferHeapCreator,
		UINT backBufferIndex);

	//ComPtr<ID3D12DescriptorHeap> GetImguiDHeap() { return imguiHeap; };
};