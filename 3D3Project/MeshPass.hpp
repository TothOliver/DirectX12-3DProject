#pragma once

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>


struct TransformConstantBuffer {
	DirectX::XMFLOAT4X4 WorldViewProjection;
};

struct CubeInstance
{
	DirectX::XMFLOAT3 Position;
	float RotationAngle = 0.0f;
	float RotationSpeed = 1.0f;
	float Scale = 1.0f;
};

class MeshPass
{
public:
	bool Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, UINT width, UINT height);
	void Draw(ID3D12GraphicsCommandList* commandList);
	void Update(float deltaTime);
	void Shutdown();

private:
	bool CreateDeviceDependantResources(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, UINT width, UINT height);
	bool CreateRootSignature(ID3D12Device* device);
	bool CreateShaders();
	bool CreatePipelineState(ID3D12Device* device, DXGI_FORMAT renderTargetFormat);
	bool CreateCubeInstances(UINT cubeCount);
	bool CreateVertexBuffer(ID3D12Device* device);
	bool CreateIndexBuffer(ID3D12Device* device);
	bool CreateConstantBuffer(ID3D12Device* device, UINT width, UINT height);
	bool CreateSRVHeap(ID3D12Device* device);

	void UpdateConstantBuffer();

private:
	static constexpr UINT CubeCount = 500;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	UINT m_indexCount = 0;

	std::vector<CubeInstance> m_cubes;
	UINT m_constantBufferStride = 0;
	uint8_t* m_mappedConstantBufferData = nullptr;

	float m_rotationAngle = 0.0f;
	float m_aspectRatio = 1.0f;

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
		DirectX::XMFLOAT2 TexCoord;
	};
};
