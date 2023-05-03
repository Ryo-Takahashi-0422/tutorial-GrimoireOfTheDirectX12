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

using namespace Microsoft::WRL;
using namespace DirectX;

#include <CreateD3DX12ResourceBuffer.h>
#include <Utility.h>
#include <PMDMaterialInfo.h>
#include <PrepareRenderingWindow.h>