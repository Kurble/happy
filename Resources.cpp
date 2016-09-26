#include "stdafx.h"
#include "Resources.h"
#include "AssetLoaders.h"

namespace happy
{
	Resources::Resources(const string basePath, RenderingContext* pRenderContext)
		: m_BasePath(basePath)
		, m_pRenderContext(pRenderContext) { }

	RenderingContext* Resources::getContext() const
	{
		return m_pRenderContext;
	}

	RenderMesh Resources::getRenderMesh(string objPath, string albedoRoughness, string normalMetallic)
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
			result = loadRenderMeshFromObj(m_pRenderContext, m_BasePath + objPath, "", "");
			m_CachedRenderMeshes.emplace_back(objPath, result);
		}
		result.setAlbedoRoughnessMap(m_pRenderContext, albedoRoughness.length() ? getTexture(albedoRoughness).m_Handle : nullptr);
		result.setNormalMetallicMap(m_pRenderContext, normalMetallic.length() ? getTexture(normalMetallic).m_Handle : nullptr);
		return result;
	}

	RenderSkin Resources::getSkin(string skinPath, string albedoRoughness, string normalMetallic)
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
			result = loadSkinFromFile(m_pRenderContext, m_BasePath + skinPath, "", "");
			m_CachedRenderSkins.emplace_back(skinPath, result);
		}
		result.setAlbedoRoughnessMap(m_pRenderContext, albedoRoughness.length() ? getTexture(albedoRoughness).m_Handle : nullptr);
		result.setNormalMetallicMap(m_pRenderContext, normalMetallic.length() ? getTexture(normalMetallic).m_Handle : nullptr);
		return result;
	}

	Animation Resources::getAnimation(string animPath)
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
			result = loadAnimFromFile(m_pRenderContext, m_BasePath + animPath);
			m_CachedAnimations.emplace_back(animPath, result);
		}
		return result;
	}

	TextureHandle Resources::getTexture(string filePath)
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

		result = loadTextureWIC(m_pRenderContext, m_BasePath + filePath);
		m_CachedTextures.emplace_back(filePath, result);
		return{ result };
	}

	TextureHandle Resources::getCubemap(string filePath[6])
	{
		string id = filePath[0];

		ComPtr<ID3D11ShaderResourceView> result;
		for (auto it = m_CachedCubemaps.begin(); it != m_CachedCubemaps.end(); ++it)
		{
			auto &cached = *it;
			if (cached.first == id)
			{
				return{ cached.second };
			}
		}

		std::string files[] = 
		{
			m_BasePath + filePath[0],
			m_BasePath + filePath[1],
			m_BasePath + filePath[2],
			m_BasePath + filePath[3],
			m_BasePath + filePath[4],
			m_BasePath + filePath[5]
		};

		result = loadCubemapWIC(m_pRenderContext, files);
		m_CachedCubemaps.emplace_back(id, result);
		return{ result };
	}

	TextureHandle Resources::getCubemapFolder(string filePath, string format)
	{
		std::string files[] =
		{
			filePath + "\\posx." + format,
			filePath + "\\negx." + format,
			filePath + "\\posy." + format,
			filePath + "\\negy." + format,
			filePath + "\\posz." + format,
			filePath + "\\negz." + format,
		};

		return getCubemap(files);
	}

	std::string Resources::getFilePath(std::string localPath)
	{
		return m_BasePath + localPath;
	}

	Canvas Resources::createCanvas(unsigned width, unsigned height)
	{
		Canvas result;

		vector<unsigned char> texture(width * height * 4, 0);
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = texture.data();
		data.SysMemPitch = width * 4;

		ComPtr<ID3D11Texture2D> textureHandle;
		THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateTexture2D(&texDesc, &data, &textureHandle));
		THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateShaderResourceView(textureHandle.Get(), nullptr, result.m_Handle.GetAddressOf()));
		THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateRenderTargetView(textureHandle.Get(),   nullptr, result.m_RenderTarget.GetAddressOf()));

		return result;
	}
}