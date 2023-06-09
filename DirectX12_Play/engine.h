#pragma once

#include <DirectXTex.h>
#include <Windows.h>
#include<tchar.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>//シェーダーコンパイルに必要
#include <d3dx12.h>
#include <string.h>
#include <map>
#include <sys/stat.h>
#include <wrl.h>
#include <unordered_map>
#include <algorithm>
#include <array>

using namespace Microsoft::WRL;
using namespace DirectX;

//#include <AppD3DX12.h>
#include <CreateD3DX12ResourceBuffer.h>
#include <Utility.h>
#include <PMDMaterialInfo.h>
#include <PrepareRenderingWindow.h>
#include <SetRootSignature.h>
#include <VMDMotionInfo.h>
#include <PMDActor.h>
#include <sstream>
//#include <AppD3DX12.h>