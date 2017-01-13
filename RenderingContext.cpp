#include "stdafx.h"
#include "RenderingContext.h"
#include "GraphicsTimer.h"

namespace happy
{
	RenderingContext::RenderingContext()
	{
		UINT flags = 0;
		#if defined(DEBUG) || defined(_DEBUG)  
		flags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif

		D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;
		THROW_ON_FAIL(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &featureLevels, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pContext));

		#if defined(DEBUG) || defined(_DEBUG)  
		ComPtr<ID3D11Debug> debug;
		THROW_ON_FAIL(m_pDevice.As(&debug));
		ComPtr<ID3D11InfoQueue> infoQueue;
		THROW_ON_FAIL(debug.As(&infoQueue));
		D3D11_MESSAGE_ID hide[] =
		{
			D3D11_MESSAGE_ID_DEVICE_DRAW_CONSTANT_BUFFER_TOO_SMALL,
			D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
			D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
			D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
			// TODO: Add more message IDs here as needed 
		};
		D3D11_INFO_QUEUE_FILTER filter;
		memset(&filter, 0, sizeof(filter));
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		infoQueue->AddStorageFilterEntries(&filter);
		#endif

		m_Width = 0;
		m_Height = 0;

		m_pGraphicsTimer = make_shared<GraphicsTimer>(*m_pDevice.Get(), *m_pContext.Get());
		m_pGraphicsTimer->begin();
	}

	void RenderingContext::attach(HWND hWnd)
	{
		IDXGIDevice2* pDXGIDevice;
		THROW_ON_FAIL(m_pDevice.Get()->QueryInterface(__uuidof(IDXGIDevice2), (void **)&pDXGIDevice));

		IDXGIAdapter* pDXGIAdapter;
		THROW_ON_FAIL(pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter));

		IDXGIFactory2* pIDXGIFactory;
		THROW_ON_FAIL(pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void **)&pIDXGIFactory));

		RECT rc;
		GetClientRect(hWnd, &rc);
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		// This is new, DXGI_SWAP_CHAIN_DESC and pIDXGIFactory->CreateSwapChain() have recently been deprecated.
		DXGI_SWAP_CHAIN_DESC1 swapChain;
		ZeroMemory(&swapChain, sizeof(swapChain));
		swapChain.Width = width;
		swapChain.Height = height;
		swapChain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChain.Stereo = FALSE;
		swapChain.SampleDesc.Count = 1;
		swapChain.SampleDesc.Quality = 0;
		swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;
		swapChain.BufferCount = 1;

		THROW_ON_FAIL(pIDXGIFactory->CreateSwapChainForHwnd(m_pDevice.Get(), hWnd, &swapChain, nullptr, nullptr, m_pSwapChain.GetAddressOf()));
		resize(width, height);
	}

	void RenderingContext::swap()
	{
		m_pSwapChain->Present(1, 0);
		m_pGraphicsTimer->end();
		m_pGraphicsTimer->begin();
	}

	void RenderingContext::resize(unsigned int width, unsigned int height)
	{
		m_Width = width;
		m_Height = height;

		ID3D11RenderTargetView* rtvs[] = { nullptr };
		m_pContext->OMSetRenderTargets(1, rtvs, nullptr);
		m_pBackBuffer = nullptr;

		// Resize backbuffer
		DXGI_SWAP_CHAIN_DESC sd;
		m_pSwapChain->GetDesc(&sd);
		THROW_ON_FAIL(m_pSwapChain->ResizeBuffers(sd.BufferCount, width, height, sd.BufferDesc.Format, 0));

		ComPtr<ID3D11Texture2D> pBackBuffer;
		THROW_ON_FAIL(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer));
		THROW_ON_FAIL(m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pBackBuffer));
		
		// Setup the viewport
		D3D11_VIEWPORT vp;
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_pContext->RSSetViewports(1, &vp);
	}

	unsigned int RenderingContext::getWidth() const
	{
		return m_Width;
	}

	unsigned int RenderingContext::getHeight() const
	{
		return m_Height;
	}

	ID3D11Device* RenderingContext::getDevice() const 
	{
		return m_pDevice.Get();
	}

	ID3D11DeviceContext* RenderingContext::getContext() const
	{
		return m_pContext.Get();
	}

	TimedDeviceContext RenderingContext::getContext(const char *perfZone) const
	{
		ID3D11Query* query = m_pGraphicsTimer->beginZone(perfZone);
		return TimedDeviceContext(m_pContext.Get(), perfZone, [this, query]		
		{
			m_pGraphicsTimer->endZone(query);
		});
	}

	ID3D11RenderTargetView* RenderingContext::getBackBuffer() const
	{
		return m_pBackBuffer.Get();
	}
}