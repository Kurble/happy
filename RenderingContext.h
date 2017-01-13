#pragma once

#include "TimedDeviceContext.h"


namespace happy
{
	class GraphicsTimer;

	class RenderingContext
	{
	public:
		RenderingContext();

		void attach(HWND hWnd);
		void resize(unsigned int width, unsigned int height);
		void swap();

		unsigned int getWidth() const;
		unsigned int getHeight() const;
		ID3D11Device* getDevice() const;
		ID3D11DeviceContext* getContext() const;
		ID3D11RenderTargetView* getBackBuffer() const;
		TimedDeviceContext getContext(const char *perfZone) const;

	private:
		ComPtr<IDXGISwapChain1> m_pSwapChain;
		ComPtr<ID3D11Device> m_pDevice;
		ComPtr<ID3D11DeviceContext> m_pContext;
		ComPtr<ID3D11RenderTargetView> m_pBackBuffer;

		shared_ptr<GraphicsTimer> m_pGraphicsTimer;

		unsigned int m_Width;
		unsigned int m_Height;
	};
}