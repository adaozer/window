#pragma once

#include "core.h"
#include "maths.h"
#include "ShaderManager.h"
#include <map>

struct alignas(16) ConstantBuffer1 {
	float time;
};

struct alignas(16) ConstantBuffer2
{
	float time;
	float padding[3];
	Vec4 lights[4];
};

struct ConstantBufferVariable
{
	unsigned int offset;
	unsigned int size;
};

class ConstantBuffer {
public:
	ID3D12Resource* constantBuffer;
	unsigned char* buffer;
	unsigned int cbSizeInBytes;
	unsigned int maxDrawCalls;
	unsigned int offsetIndex;

	std::string name;
	std::map<std::string, ConstantBufferVariable> constantBufferData;

	void init(Core* core, unsigned int sizeInBytes, int frames) {
		cbSizeInBytes = (sizeInBytes + 255) & ~255;
		HRESULT hr;
		D3D12_HEAP_PROPERTIES heapprops = {};
		heapprops.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapprops.CreationNodeMask = 1;
		heapprops.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC cbDesc = {};
		cbDesc.Width = cbSizeInBytes * frames;
		cbDesc.Height = 1;
		cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbDesc.DepthOrArraySize = 1;
		cbDesc.MipLevels = 1;
		cbDesc.SampleDesc.Count = 1;
		cbDesc.SampleDesc.Quality = 0;
		cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		hr = core->device->CreateCommittedResource(&heapprops, D3D12_HEAP_FLAG_NONE, &cbDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void**)&constantBuffer);
		hr = constantBuffer->Map(0, NULL, (void**)&buffer);
	}

	void initShader(ShaderManager* shader) {

		ID3D12ShaderReflection* reflection;
		D3DReflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflection));
		D3D12_SHADER_DESC desc;
		reflection->GetDesc(&desc);

		for (int i = 0; i < desc.ConstantBuffers; i++)
		{
			ConstantBuffer buffer;
			ID3D12ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			constantBuffer->GetDesc(&cbDesc);
			buffer.name = cbDesc.Name;
			unsigned int totalSize = 0;
			for (int j = 0; j < cbDesc.Variables; j++)
			{
				ID3D12ShaderReflectionVariable* var = constantBuffer->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC vDesc;
				var->GetDesc(&vDesc);
				ConstantBufferVariable bufferVariable;
				bufferVariable.offset = vDesc.StartOffset;
				bufferVariable.size = vDesc.Size;
				buffer.constantBufferData.insert({ vDesc.Name, bufferVariable });
				totalSize += bufferVariable.size;
			}
		}
	}

	void update(void* data, unsigned int sizeInBytes, int frame) {
		memcpy(buffer + (frame * cbSizeInBytes), data, sizeInBytes);
	}

	D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress(int frame) {
		return (constantBuffer->GetGPUVirtualAddress() + (frame * cbSizeInBytes));
	}
};