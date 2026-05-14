#pragma once

#include <d3d12.h>
#include <wrl/client.h>

class MeshPass
{
public:
	bool Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, UINT width, UINT height);
	void Draw(ID3D12GraphicsCommandList* commandList);
	void Shutdown();

private:
	bool CreateDeviceDependantResources(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, UINT width, UINT height);
	bool CreateRootSignature(ID3D12Device* device);
	bool CreateShaders();
	bool CreatePipelineState(ID3D12Device* device, DXGI_FORMAT renderTargetFormat);
	bool CreateVertexBuffer(ID3D12Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	struct Vertex
	{
		float position[3];
		float color[4];
	};
};
