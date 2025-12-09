#pragma once
#include <string>
#include "core.h"
#include <unordered_map>

class Texture {
public:
	ID3D12Resource* tex;
	int heapOffset;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	void upload(Core* core, int width, int height, int channels, void* data);
	void load(Core* core, std::string filename);
};

class TextureManager {
public:
	std::unordered_map<std::string, Texture*> textures;
	Texture* loadTexture(Core* core, std::string filename);
}; 