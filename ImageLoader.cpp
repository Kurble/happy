#include "stdafx.h"
#include "AssetLoaders.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace happy
{
	class Image
	{
	public:
		Image(fs::path &path)
		{
			int n;
			m_Data = stbi_load(path.string().c_str(), &m_Width, &m_Height, &n, 4);
		}
		~Image()
		{
			stbi_image_free(m_Data);
		}

		unsigned getWidth() const { return (unsigned)m_Width; }
		unsigned getHeight() const { return (unsigned)m_Height; }
		unsigned char* getData() const { return m_Data; }

	private:
		int m_Width;
		int m_Height;
		unsigned char* m_Data;
	};

	ComPtr<ID3D11ShaderResourceView> loadCubemapWIC(RenderingContext *pRenderContext, fs::path filePath[6])
	{
		ComPtr<ID3D11Texture2D> pCubemap;
		{
			Image faces[6] = { filePath[0], filePath[1], filePath[2], filePath[3], filePath[4], filePath[5] };

			for (int i = 1; i < 6; ++i)
				if (faces[i].getWidth() != faces[0].getWidth() ||
					faces[i].getHeight() != faces[0].getHeight())
					throw exception("All faces of a cubemap must be the same size");

			D3D11_TEXTURE2D_DESC desc;
			desc.Width = faces[0].getWidth();
			desc.Height = faces[0].getHeight();
			desc.MipLevels = 1;
			desc.ArraySize = 6;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.CPUAccessFlags = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			D3D11_SUBRESOURCE_DATA data[6];
			for (unsigned int i = 0; i < 6; ++i)
			{
				data[i].pSysMem = faces[i].getData();
				data[i].SysMemPitch = faces[i].getWidth() * 4;
				data[i].SysMemSlicePitch = 0;
			}

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateTexture2D(&desc, data, &pCubemap));

#ifdef DEBUG
			std::string tag = filePath[0].filename().string();
			pCubemap->SetPrivateData(WKPDID_D3DDebugObjectName, tag.length(), tag.data());
#endif
		}

		ComPtr<ID3D11ShaderResourceView> pSRV;
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MipLevels = 1;
			desc.TextureCube.MostDetailedMip = 0;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateShaderResourceView(pCubemap.Get(), &desc, &pSRV));
		}

		return pSRV;
	}

	ComPtr<ID3D11ShaderResourceView> loadCubemapWICFolder(RenderingContext *pRenderContext, fs::path folderPath, std::string format)
	{
		fs::path files[] =
		{
			folderPath / ("posx." + format),
			folderPath / ("negx." + format),
			folderPath / ("posy." + format),
			folderPath / ("negy." + format),
			folderPath / ("posz." + format),
			folderPath / ("negz." + format),
		};
		return loadCubemapWIC(pRenderContext, files);
	}

	ComPtr<ID3D11ShaderResourceView> loadTextureWIC(RenderingContext *pRenderContext, fs::path filePath)
	{
		Image image = filePath;

		ComPtr<ID3D11Texture2D> pTexture;
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = image.getWidth();
			desc.Height = image.getHeight();
			desc.MipLevels = 0;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateTexture2D(&desc, nullptr, &pTexture));

#ifdef DEBUG
			std::string tag = filePath.filename().string();
			pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, tag.length(), tag.data());
#endif
		}
		
		ComPtr<ID3D11ShaderResourceView> pSRV;
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = -1;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateShaderResourceView(pTexture.Get(), &desc, &pSRV));
		}

		pRenderContext->getContext()->UpdateSubresource(pTexture.Get(), 0, nullptr, image.getData(), image.getWidth() * 4, image.getWidth()*image.getHeight()*4);
		pRenderContext->getContext()->GenerateMips(pSRV.Get());
	
		return pSRV;
	}

	ComPtr<ID3D11ShaderResourceView> loadTextureCombinedWIC(RenderingContext *pRenderContext, unsigned defaultPixel, vector<TextureWithFilter> files)
	{
		vector<Image> images;
		images.reserve(files.size());
		
		for (auto &entry : files)
			images.emplace_back(entry.m_path);

		unsigned width = images[0].getWidth();
		unsigned height = images[0].getHeight();

		vector<unsigned> combinedImageData;
		combinedImageData.resize(width * height, defaultPixel);
		
		for (unsigned _image = 0; _image < images.size(); ++_image)
		{
			auto &image = images[_image];

			if (image.getWidth() != width || image.getHeight() != height)
				throw exception("images must be same size");

			unsigned mask = files[_image].m_mask;
			unsigned shift = abs(files[_image].m_shift);
			unsigned *srcdata = reinterpret_cast<unsigned*>(image.getData());
			unsigned *dstdata = combinedImageData.data();
			unsigned length = combinedImageData.size();

			if (files[_image].m_shift >= 0)
			{
				for (unsigned pixel = 0; pixel < length; ++pixel)
				{
					dstdata[pixel] += (srcdata[pixel] & mask) >> shift;
				}
			}
			else
			{
				for (unsigned pixel = 0; pixel < length; ++pixel)
				{
					dstdata[pixel] += (srcdata[pixel] & mask) << shift;
				}
			}
		}

		ComPtr<ID3D11Texture2D> pTexture;
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = images[0].getWidth();
			desc.Height = images[0].getHeight();
			desc.MipLevels = 0;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateTexture2D(&desc, nullptr, &pTexture));

#ifdef DEBUG
			std::string tag = filesWithMapping[0].first.filename().string();
			pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, tag.length(), tag.data());
#endif
		}

		ComPtr<ID3D11ShaderResourceView> pSRV;
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipLevels = -1;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateShaderResourceView(pTexture.Get(), &desc, &pSRV));
		}

		pRenderContext->getContext()->UpdateSubresource(pTexture.Get(), 0, nullptr, (void*)combinedImageData.data(), width*4, width*height*4);
		pRenderContext->getContext()->GenerateMips(pSRV.Get());

		return pSRV;
	}
}