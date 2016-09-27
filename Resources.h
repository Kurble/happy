#pragma once

#include "RenderMesh.h"
#include "RenderSkin.h"
#include "Animation.h"
#include "TextureHandle.h"
#include "Canvas.h"

namespace happy
{
	class Resources
	{
	public:
		Resources(const string basePath, RenderingContext *pRenderContext);

		RenderingContext* getContext() const;

		RenderMesh getRenderMesh(std::string objPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		RenderSkin getSkin(std::string skinPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		Animation getAnimation(std::string animationPath);

		TextureHandle getCubemap(std::string filePath[6]);

		TextureHandle getCubemapFolder(std::string folderPath, std::string format);

		TextureHandle getTexture(std::string filePath);

		std::string getFilePath(std::string localPath);

		Canvas createCanvas(unsigned width, unsigned height);

	private:
		RenderingContext *m_pRenderContext;
		string m_BasePath;

		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedCubemaps;
		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedTextures;
		vector<pair<string, RenderMesh>> m_CachedRenderMeshes;
		vector<pair<string, RenderSkin>> m_CachedRenderSkins;
		vector<pair<string, Animation>> m_CachedAnimations;
	};

	template <typename T, size_t Length> void CreateVertexShader(ID3D11Device* device, ComPtr<ID3D11VertexShader> &vs, ComPtr<ID3D11InputLayout> &il, const BYTE(&shaderByteCode)[Length])
	{
		size_t length = Length;
		THROW_ON_FAIL(device->CreateVertexShader(shaderByteCode, length, nullptr, &vs));
		THROW_ON_FAIL(device->CreateInputLayout(T::Elements, T::ElementCount, shaderByteCode, length, &il));
	}

	template <size_t Length> void CreatePixelShader(ID3D11Device* device, ComPtr<ID3D11PixelShader> &ps, const BYTE(&shaderByteCode)[Length])
	{
		size_t length = Length;
		THROW_ON_FAIL(device->CreatePixelShader(shaderByteCode, length, nullptr, &ps));
	}
}