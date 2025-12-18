#pragma once
#include "GEMLoader.h"
#include "mesh.h"
#include "Texture.h"

class InstancedStaticMesh {
public:
	GEMLoader::GEMModelLoader loader;
	std::vector<GEMLoader::GEMMesh> gemmeshes;
	std::vector<Mesh*> meshes;
	std::vector<INSTANCE> instances;

	void setInstances(const std::vector<INSTANCE>& _instances) {
		instances = _instances; // initialise instance vector
	}

	void load(Core* core, std::string filename) {
		loader.load(filename, gemmeshes);
		for (int i = 0; i < gemmeshes.size(); i++) {
			Mesh* mesh = new Mesh();
			std::vector<STATIC_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++) {
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, instances, gemmeshes[i].indices); // initialise mesh using the instance initialiser
			meshes.push_back(mesh);
		}
	}

	void draw(Core* core, Texture* texture, ShaderManager* shaderManager) {
		for (int i = 0; i < meshes.size(); i++) {
			shaderManager->updateTexturePS(core, "diffuseTex", texture->heapOffset); // bind the textures to the pixel shader
			// other texture not necessary to bind as they're at adjacent memory.
			meshes[i]->draw(core); // draw
		}
	}
};

class InstancedObject {
public:
	ShaderManager* shaderManager;
	PSOManager psos; // Shader manager, PSO Manager, Texture Manager etc initialised
	VertexLayoutCache vertexLayoutCache;
	Shader* vertexShader = nullptr;
	Shader* pixelShader = nullptr; // 2 different shaders (vertex and pixel)
	InstancedStaticMesh sm; 
	TextureManager* textureManager;
	Texture* diffuseTex = nullptr;
	Texture* normalTex = nullptr;
	std::string filepath;
	std::string filepath2;
	std::string psType; // 2 textures for diffuse textures and normal mapping. 2 different pixelshader types in case normal mapping isn't required

	InstancedObject(ShaderManager* shadermanager, TextureManager* tx, std::string _filepath, std::string _filepath2, std::string _psType) : shaderManager(shadermanager), 
		textureManager(tx), filepath(_filepath), filepath2(_filepath2), psType(_psType) {}

	void init(Core* core, std::string filename, std::vector<INSTANCE>& instances) {
		sm.setInstances(instances); // constructor for instances vector
		sm.load(core, filename); // load the meshes

		vertexShader = shaderManager->loadShader(core, "vertexshader_instanced.hlsl", true);
		pixelShader = shaderManager->loadShader(core, psType, false);
		diffuseTex = textureManager->loadTexture(core, filepath);
		normalTex = textureManager->loadTexture(core, filepath2); // Initialise shaders
		psos.createPSO(core, "InstancedGEM", vertexShader->shader, pixelShader->shader, vertexLayoutCache.getInstancedLayout()); // Create pso
	}

	void draw(Core* core, Matrix& VP) {
		psos.bind(core, "InstancedGEM"); // bind PSO
		shaderManager->updateConstantVS("vertexshader_instanced.hlsl", "staticMeshBuffer", "VP", &VP); // update vp matrix. No need to update W as this is an instance object
		vertexShader->apply(core); // apply updates
		sm.draw(core, diffuseTex, shaderManager); // draw
	}
};