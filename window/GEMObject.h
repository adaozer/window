#pragma once
#include "GEMLoader.h"
#include "mesh.h"

class GEMObject {
public:
	GEMLoader::GEMModelLoader loader;
	std::vector<GEMLoader::GEMMesh> gemmeshes;
	std::vector<Mesh> meshes;
	std::vector<STATIC_VERTEX> vertices;
	Mesh mesh;
	ShaderManager* shaderManager;
	PSOManager psos;
	VertexLayoutCache vertexLayoutCache;
	Shader* vertexShader = nullptr;
	Shader* pixelShader = nullptr;

	GEMObject(ShaderManager* sm) : shaderManager(sm) {}

	void init(Core* core) {
		loader.load("acacia_003.gem", gemmeshes);
		for (int i = 0; i < gemmeshes.size(); i++) {
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) {
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			mesh.init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
			vertexShader = shaderManager->loadShader(core, "vertexshader.hlsl", true);
			pixelShader = shaderManager->loadShader(core, "pixelshader.hlsl", false);
			psos.createPSO(core, "GEMObject", vertexShader->shader, pixelShader->shader, vertexLayoutCache.getStaticLayout());
		}
	}

	void draw(Core* core, Matrix& W, Matrix& VP) {
		core->beginRenderPass();
		for (int i = 0; i < meshes.size(); i++) {
			psos.bind(core, "GEMObject");

			shaderManager->updateConstantVS("vertexshader.hlsl", "staticMeshBuffer", "W", &W);
			shaderManager->updateConstantVS("vertexshader.hlsl", "staticMeshBuffer", "VP", &VP);

			vertexShader->apply(core);
			meshes[i].draw(core);
		}
	}

};