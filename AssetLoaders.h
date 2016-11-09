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

	ComPtr<ID3D11ShaderResourceView> loadCubemapWIC(RenderingContext *pRenderContext, fs::path filePath[6]);

	ComPtr<ID3D11ShaderResourceView> loadCubemapWICFolder(RenderingContext *pRenderContext, fs::path folderPath, std::string format);

	ComPtr<ID3D11ShaderResourceView> loadTextureWIC(RenderingContext *pRenderContext, fs::path filePath);
	
	struct TextureWithFilter
	{
		fs::path m_path;
		unsigned m_mask;
		int m_shift;
	};

	ComPtr<ID3D11ShaderResourceView> loadTextureCombinedWIC(RenderingContext *pRenderContext, unsigned defaultPixel, vector<TextureWithFilter> files);

}