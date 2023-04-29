#include <stdafx.h>
#include"AppD3DX12.h"

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	auto& app = AppD3DX12::Instance();
	if (!app.Init()) {
		return -1;
	}
	//app.Run();
	//app.Terminate();
	return 0;
}