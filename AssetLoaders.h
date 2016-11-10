#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "RenderSkin.h"
#include "Animation.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace happy
{
	
	RenderMesh loadRenderMeshFromObj(RenderingContext *pRenderContext, fs::path objPath, fs::path albedoRoughnessPath, fs::path normalMetallicPath);

	RenderSkin loadSkinFromFile(RenderingContext *pRenderContext, fs::path skinPath, fs::path albedoRoughnessPath, fs::path normalMetallicPath);

	Animation loadAnimFromFile(RenderingContext *pRenderContext, fs::path animPath);

	ComPtr<ID3D11ShaderResourceView> loadCubemap(RenderingContext *pRenderContext, fs::path filePath[6]);

	ComPtr<ID3D11ShaderResourceView> loadCubemapFolder(RenderingContext *pRenderContext, fs::path folderPath, std::string format);

	ComPtr<ID3D11ShaderResourceView> loadTexture(RenderingContext *pRenderContext, fs::path filePath);
	
	struct TextureLayer
	{
		fs::path m_path;

		enum { rgb, gray } type;
		enum { r, g, b, a } target;
	};

	ComPtr<ID3D11ShaderResourceView> loadCombinedTexture(RenderingContext *pRenderContext, unsigned defaultPixel, vector<TextureLayer> files);

}