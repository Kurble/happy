#include "stdafx.h"
#include "Resources.h"
#include "AssetLoaders.h"

namespace happy
{
	Resources::Resources(const fs::path basePath, RenderingContext* pRenderContext)
		: m_BasePath(basePath)
		, m_pRenderContext(pRenderContext) { }

	RenderingContext* Resources::getContext() const
	{
		return m_pRenderContext;
	}

	RenderMesh Resources::getRenderMesh(fs::path objPath, fs::path albedoRoughness, fs::path normalMetallic)
	{
		RenderMesh result;
		bool found = false;

		for (auto it = m_CachedRenderMeshes.begin(); it != m_CachedRenderMeshes.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == objPath)
			{
				result = cached.second;
				found = true;
			}
		}

		if (!found)
		{
			result = loadRenderMeshFromObj(m_pRenderContext, m_BasePath / objPath, "", "");
			m_CachedRenderMeshes.emplace_back(objPath, result);
		}
		result.setAlbedoRoughnessMap(m_pRenderContext, albedoRoughness.empty() ? nullptr : getTexture(albedoRoughness).m_Handle);
		result.setNormalMetallicMap(m_pRenderContext, normalMetallic.empty() ? nullptr : getTexture(normalMetallic).m_Handle);
		return result;
	}

	RenderSkin Resources::getSkin(fs::path skinPath, fs::path albedoRoughness, fs::path normalMetallic)
	{
		RenderSkin result;
		bool found = false;

		for (auto it = m_CachedRenderSkins.begin(); it != m_CachedRenderSkins.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == skinPath)
			{
				result = cached.second;
				found = true;
			}
		}

		if (!found)
		{
			result = loadSkinFromFile(m_pRenderContext, m_BasePath / skinPath, "", "");
			m_CachedRenderSkins.emplace_back(skinPath, result);
		}
		result.setAlbedoRoughnessMap(m_pRenderContext, albedoRoughness.empty() ? nullptr : getTexture(albedoRoughness).m_Handle);
		result.setNormalMetallicMap(m_pRenderContext, normalMetallic.empty() ? nullptr : getTexture(normalMetallic).m_Handle);
		return result;
	}

	Animation Resources::getAnimation(fs::path animPath)
	{
		Animation result;
		bool found = false;

		for (auto it = m_CachedAnimations.begin(); it != m_CachedAnimations.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == animPath)
			{
				result = cached.second;
				found = true;
			}
		}

		if (!found)
		{
			result = loadAnimFromFile(m_pRenderContext, m_BasePath / animPath);
			m_CachedAnimations.emplace_back(animPath, result);
		}
		return result;
	}

	TextureHandle Resources::getTexture(fs::path filePath)
	{
		ComPtr<ID3D11ShaderResourceView> result;
		for (auto it = m_CachedTextures.begin(); it != m_CachedTextures.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == filePath)
			{
				return{ cached.second };
			}
		}

		result = loadTextureWIC(m_pRenderContext, m_BasePath / filePath);
		m_CachedTextures.emplace_back(filePath, result);
		return{ result };
	}

	TextureHandle Resources::getCubemap(fs::path filePath[6])
	{
		fs::path id = filePath[0];

		ComPtr<ID3D11ShaderResourceView> result;
		for (auto it = m_CachedCubemaps.begin(); it != m_CachedCubemaps.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == id)
			{
				return{ cached.second };
			}
		}

		fs::path files[] =
		{
			m_BasePath / filePath[0],
			m_BasePath / filePath[1],
			m_BasePath / filePath[2],
			m_BasePath / filePath[3],
			m_BasePath / filePath[4],
			m_BasePath / filePath[5]
		};

		result = loadCubemapWIC(m_pRenderContext, files);
		m_CachedCubemaps.emplace_back(id, result);
		return{ result };
	}

	TextureHandle Resources::getCubemapFolder(fs::path filePath, std::string format)
	{
		fs::path files[] =
		{
			filePath / ("posx." + format),
			filePath / ("negx." + format),
			filePath / ("posy." + format),
			filePath / ("negy." + format),
			filePath / ("posz." + format),
			filePath / ("negz." + format),
		};

		return getCubemap(files);
	}

	fs::path Resources::getFilePath(fs::path localPath)
	{
		return m_BasePath / localPath;
	}

	Canvas Resources::createCanvas(unsigned width, unsigned height, bool monoColor)
	{
		Canvas result;
		result.load(m_pRenderContext, width, height, monoColor ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM);
		return result;
	}
}