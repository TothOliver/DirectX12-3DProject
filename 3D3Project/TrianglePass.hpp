#pragma once

#include <d3d12.h>
#include <wrl/client.h>

class TriangleRenderer
{
public:
	bool Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat);
	void Draw(ID3D12GraphicsCommandList* commandList);
	void Shutdown();
};
