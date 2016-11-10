#include "stdafx.h"
#include "Resources.h"
#include "AssetLoaders.h"
#include "bb_lib\store.h"

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

		result = loadTexture(m_pRenderContext, m_BasePath / filePath);
		m_CachedTextures.emplace_back(filePath, result);
		return{ result };
	}

	MultiTexture Resources::getMultiTexture(fs::path descFilePath)
	{
		for (auto it = m_CachedMultiTextures.begin(); it != m_CachedMultiTextures.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == descFilePath)
			{
				return cached.second;
			}
		}

		MultiTexture result;
		
		bb::store desc;
		auto load_channel = [&](std::string name, ComPtr<ID3D11ShaderResourceView> &target)
		{
			if (desc.exists(name))
			{
				auto &table = desc.getFieldD(name);
				unsigned default = (unsigned)table.getFieldI("default");
				
				vector<TextureLayer> sources;

				if (table.exists("sources")) for (auto &v : table.getFieldL("sources"))
				{
					auto &source = v.d;
					sources.push_back(TextureLayer());
					
					sources.back().m_path = source.getFieldS("file");

					std::string type = source.getFieldS("type");
					if      (type == "rgb")  sources.back().type = TextureLayer::rgb;
					else if (type == "gray") sources.back().type = TextureLayer::gray;
					else throw exception("invalid source type");

					std::string target = source.getFieldS("target");
					if      (target == "r") sources.back().target = TextureLayer::r;
					else if (target == "g") sources.back().target = TextureLayer::g;
					else if (target == "b") sources.back().target = TextureLayer::b;
					else if (target == "a") sources.back().target = TextureLayer::a;
					else throw exception("invalid source type");
				}

				target = loadCombinedTexture(m_pRenderContext, default, sources);
			}
		};

		load_channel("channel0", result.m_Channels[0]);
		load_channel("channel1", result.m_Channels[1]);
		load_channel("channel2", result.m_Channels[2]);

		m_CachedMultiTextures.emplace_back(descFilePath, result);
		return result;
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

		result = loadCubemap(m_pRenderContext, files);
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