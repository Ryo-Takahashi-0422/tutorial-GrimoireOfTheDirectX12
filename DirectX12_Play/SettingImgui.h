#pragma once

class SettingImgui
{
private:
	ComPtr<ID3D12DescriptorHeap> imguiSRVHeap = nullptr; // imguiのSRV用ディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> imguiRTVHeap = nullptr; // imguiのRTV用ディスクリプタヒープ
	ComPtr<ID3D12Resource> imguiBuff = nullptr; // imgui用バッファー

	HRESULT CreateSRVDHeap4Imgui(ComPtr<ID3D12Device> _dev);
	HRESULT CreateRTVDHeap4Imgui(ComPtr<ID3D12Device> _dev);
	HRESULT CreateBuff4Imgui(ComPtr<ID3D12Device> _dev);
	void CreateRTV4Imgui(ComPtr<ID3D12Device> _dev);
	bool blnResult;

public:
	// マルチパスSRV用ディスクリプタヒープの作成
	HRESULT Init(ComPtr<ID3D12Device> _dev,	PrepareRenderingWindow* pRWindow);

	void DrawDateOfImGUI(
		ComPtr<ID3D12Device> _dev,
		ComPtr<ID3D12GraphicsCommandList> _cmdList,
		std::vector<ComPtr<ID3D12Resource>> pResoures,
		BufferHeapCreator*/*std::vector<BufferHeapCreator*>*/ bufferHeapCreator,
		UINT backBufferIndex);

	//ComPtr<ID3D12DescriptorHeap> GetImguiDHeap() { return imguiHeap; };
};