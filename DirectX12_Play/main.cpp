#include <stdafx.h>
#include"AppD3DX12.h"

#ifdef _DEBUG
int main() {
	PIXLoadLatestWinPixGpuCapturerLibrary();
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif

	auto& app = AppD3DX12::Instance();

	if (!app.PrepareRendering()) {
		return -1;
	}
	
	if (!app.PipelineInit()) {
		return -1;
	}

	if (!app.ResourceInit()) {
		return -1;
	}
	
	app.Run();
	//app.Terminate();
	return 0;
}