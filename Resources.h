#pragma once

#include "RenderMesh.h"
#include "RenderSkin.h"
#include "Animation.h"
#include "TextureHandle.h"
#include "MultiTexture.h"
#include "PostProcessItem.h"
#include "SurfaceShader.h"
#include "Canvas.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace happy
{
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

	class Resources
	{
	public:
		Resources(const fs::path basePath, RenderingContext *pRenderContext);

		RenderingContext* getContext() const;

		shared_ptr<RenderMesh> getMesh(const fs::path &filePath, const fs::path &multitextureDef);

		Animation getAnimation(const fs::path &animationPath);

		TextureHandle getCubemap(const fs::path filePath[6]);

		TextureHandle getCubemapFolder(const fs::path &folderPath, const std::string &format);

		TextureHandle getTexture(const fs::path &filePath);

		MultiTexture getMultiTexture(const fs::path &descFilePath);

		fs::path getFilePath(const fs::path &localPath);

		template<size_t Length>
		PostProcessItem createPostProcess(const BYTE(&shaderByteCode)[Length])
		{
			PostProcessItem result;

			// Create shader
			CreatePixelShader(m_pRenderContext->getDevice(), result.m_Handle, shaderByteCode);

			return result;
		}

		template <size_t Length>
		SurfaceShader createSurfaceShader(const BYTE(&shaderByteCode)[Length])
		{
			SurfaceShader result;

			// Create shader
			CreatePixelShader(m_pRenderContext->getDevice(), result.m_Handle, shaderByteCode);

			return result;
		}

		template<typename T, size_t Length>
		ConstBufPostProcessItem<T> createCBPostProcess(const BYTE(&shaderByteCode)[Length])
		{
			ConstBufPostProcessItem<T> result;

			// Create shader
			CreatePixelShader(m_pRenderContext->getDevice(), result.m_Handle, shaderByteCode);

			// Create constbuf data
			result.m_ConstBufferData = make_shared<vector<unsigned char>>(((sizeof(T) + 15) / 16) * 16);

			// Create constbuf
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = ((sizeof(T) + 15) / 16) * 16;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = result.m_ConstBufferData->data();
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateBuffer(&desc, &data, result.m_ConstBuffer.GetAddressOf()));

			return result;
		}

		Canvas createCanvas(unsigned width, unsigned height, bool monoColor);

	private:
		RenderingContext *m_pRenderContext;
		fs::path m_BasePath;

		vector<pair<fs::path, ComPtr<ID3D11ShaderResourceView>>> m_CachedCubemaps;
		vector<pair<fs::path, ComPtr<ID3D11ShaderResourceView>>> m_CachedTextures;
		vector<pair<fs::path, MultiTexture>> m_CachedMultiTextures;
		vector<pair<fs::path, shared_ptr<RenderMesh>>> m_CachedRenderMeshes;
		vector<tuple<fs::path, fs::path, shared_ptr<RenderMesh>>> m_CachedTexturedRenderMeshes;
		vector<pair<fs::path, Animation>> m_CachedAnimations;
	};
}