#pragma once
#include "meshes.h"
#include "buffering.h"

class Triangle {
public:
	ScreenSpaceTriangle sst;
	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	PSOManager psos;
	ConstantBuffer constantBuffer;

	void init(Core* core) {
		sst.init(core);
		ID3DBlob* status;
		std::string vertexshader = readFile("vertexshader.hlsl");
		HRESULT hr = D3DCompile(vertexshader.c_str(), strlen(vertexshader.c_str()), NULL,
			NULL, NULL, "VS", "vs_5_0", 0, 0, &vertexShader, &status);
		std::string pixelshader = readFile("pixelshader.hlsl");
		hr = D3DCompile(pixelshader.c_str(), strlen(pixelshader.c_str()), NULL, NULL,
			NULL, "PS", "ps_5_0", 0, 0, &pixelShader, &status);

		if (FAILED(hr)) (char*)status->GetBufferPointer();
		psos.createPSO(core, "Triangle", vertexShader, pixelShader, sst.m.inputLayoutDesc);
		constantBuffer.init(core, sizeof(ConstantBuffer2), 2);
	}

	std::string readFile(std::string filename) {
		std::ifstream file(filename);
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	void draw(Core* core, ConstantBuffer2* cb) {
		core->beginRenderPass();
		constantBuffer.update(cb, sizeof(ConstantBuffer2), core->frameIndex());
		core->getCommandList()->SetGraphicsRootConstantBufferView(1, constantBuffer.getGPUAddress(core->frameIndex()));
		psos.bind(core, "Triangle");
		sst.m.draw(core);
	}
};
