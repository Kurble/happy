#include "stdafx.h"
#include "RenderTarget.h"

namespace happy
{
	RenderTarget::RenderTarget(RenderingContext *context, unsigned width, unsigned height, bool hiresEffects) 
		: m_pRenderContext(context)
		, m_pOutputTarget(nullptr)
		, m_Jitter(0.33f, 0.33f)
	{
		if (width > 0 && height > 0)
			resize(width, height, hiresEffects);
	}

	void RenderTarget::resize(unsigned width, unsigned height, bool hiresEffects)
	{
		ID3D11Device& device = *m_pRenderContext->getDevice();

		m_ViewPort.Width = (float)width;
		m_ViewPort.Height = (float)height;
		m_ViewPort.MinDepth = 0.0f;
		m_ViewPort.MaxDepth = 1.0f;
		m_ViewPort.TopLeftX = 0;
		m_ViewPort.TopLeftY = 0;

		m_BlurViewPort = m_ViewPort;
		if (!hiresEffects)
		{
			m_BlurViewPort.Width = (float)width / 2.0f;
			m_BlurViewPort.Height = (float)height / 2.0f;
		}

		// Create G-Buffer
		vector<unsigned char> texture(width * height * 8, 0);
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = texture.data();
		data.SysMemPitch = width * 8;

		for (size_t i = 0; i < GBuf_ChannelCount; ++i)
		{
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

			if (i == GBuf_DepthStencilIdx)
			{
				continue;
			}
			else if (i == GBuf_Occlusion0Idx || i == GBuf_Occlusion1Idx)
			{
				if (!hiresEffects)
				{
					texDesc.Width = width / 2;
					texDesc.Height = height / 2;
				}
				texDesc.Format = DXGI_FORMAT_R16_UNORM;
			}
			else if (i == GBuf_Graphics1Idx)
			{
				texDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
			}

			ComPtr<ID3D11Texture2D> tex;
			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &tex));
			THROW_ON_FAIL(device.CreateRenderTargetView(tex.Get(), nullptr, m_GraphicsBuffer[i].rtv.GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(tex.Get(), nullptr, m_GraphicsBuffer[i].srv.GetAddressOf()));
		}

		ComPtr<ID3D11Texture2D> depthStencilTex;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &depthStencilTex));

		// depth-stencil buffer object
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = 0;
			THROW_ON_FAIL(device.CreateDepthStencilView(depthStencilTex.Get(), &dsvDesc, m_pDepthBufferView.GetAddressOf()));
		}

		// depth-stencil buffer object (read only)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
			THROW_ON_FAIL(device.CreateDepthStencilView(depthStencilTex.Get(), &dsvDesc, m_pDepthBufferViewReadOnly.GetAddressOf()));
		}

		// depth buffer view
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1;
			THROW_ON_FAIL(device.CreateShaderResourceView(depthStencilTex.Get(), &srvDesc, m_GraphicsBuffer[GBuf_DepthStencilIdx].srv.GetAddressOf()));
		}

		// post buffer
		for (unsigned int i = 0; i < 2; ++i)
		{
			ComPtr<ID3D11Texture2D> tex;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &tex));
			THROW_ON_FAIL(device.CreateRenderTargetView(tex.Get(), nullptr, m_PostBuffer[i].rtv.GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(tex.Get(), nullptr, m_PostBuffer[i].srv.GetAddressOf()));
		}

		// history buffer
		for (unsigned int i = 0; i < 2; ++i)
		{
			ComPtr<ID3D11Texture2D> tex;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &tex));
			THROW_ON_FAIL(device.CreateRenderTargetView(tex.Get(), nullptr, m_HistoryBuffer[i].rtv.GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(tex.Get(), nullptr, m_HistoryBuffer[i].srv.GetAddressOf()));
		}
	}

	const RenderingContext* RenderTarget::getContext() const
	{
		return m_pRenderContext;
	}

	const bb::mat4& RenderTarget::getView() const
	{
		return m_View;
	}

	const bb::mat4& RenderTarget::getProjection() const
	{
		return m_Projection;
	}
	
	const float RenderTarget::getWidth() const
	{
		return m_ViewPort.Width;
	}

	const float RenderTarget::getHeight() const
	{
		return m_ViewPort.Height;
	}

	void RenderTarget::setView(bb::mat4 &view)
	{
		m_View = view;
	}

	void RenderTarget::setProjection(bb::mat4 &projection)
	{
		m_Projection = projection;
	}

	void RenderTarget::setOutput(ID3D11RenderTargetView* target)
	{
		m_pOutputTarget = target;
	}

	ID3D11RenderTargetView* RenderTarget::historyRTV() const
	{
		unsigned i = (m_LastUsedHistoryBuffer + 1) % 2;
		return m_HistoryBuffer[i].rtv.Get();
	}

	ID3D11ShaderResourceView* RenderTarget::historySRV() const
	{
		unsigned i = m_LastUsedHistoryBuffer % 2;
		return m_HistoryBuffer[i].srv.Get();
	}

	ID3D11ShaderResourceView* RenderTarget::currentSRV() const
	{
		unsigned i = (m_LastUsedHistoryBuffer + 1) % 2;
		return m_HistoryBuffer[i].srv.Get();
	}
}