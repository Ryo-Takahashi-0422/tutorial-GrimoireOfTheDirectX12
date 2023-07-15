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
using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

//#include <AppD3DX12.h>
#include <VertexInputLayout.h>
#include <CreateD3DX12ResourceBuffer.h>
#include <Utility.h>
#include <PMDMaterialInfo.h>
#include <PrepareRenderingWindow.h>
#include <SetRootSignature.h>
#include <SettingShaderCompile.h>
#include <VMDMotionInfo.h>
#include <PMDActor.h>
#include <IGraphicsPipelineSetting.h>
#include <GraphicsPipelineSetting.h>
#include <TextureLoader.h>
#include <BufferHeapCreator.h>
#include <TextureTransporter.h>
#include <MappingExecuter.h>
#include <ViewCreator.h>
#include <sstream>
//#include <AppD3DX12.h>

#include <PeraPolygon.h> // ﾏﾙﾁﾊﾟｽテスト用
#include <PeraLayout.h>
#include <PeraShaderCompile.h>
#include <PeraGraphicsPipelineSetting.h>