#pragma once

#include "TimedDeviceContext.h"

namespace vk
{
	class PhysicalDevice;
	class Device;
	class RenderPass;
}

namespace happy
{
	class GraphicsTimer;

	class RenderingContext
	{
	public:
		RenderingContext(HINSTANCE, HWND);

		void resize(unsigned int width, unsigned int height);
		void acquire();
		void present();

		unsigned int getWidth() const;
		unsigned int getHeight() const;
		//vk::PhysicalDevice* getPhysicalDevice() const;
		//vk::Device* getDevice() const;
		//vk::RenderPass* getBackBuffer() const;

		ID3D11Device* getDevice() const;
		ID3D11DeviceContext* getContext() const;
		ID3D11RenderTargetView* getBackBuffer() const;
		TimedDeviceContext RenderingContext::getContext(const char *zone) const;


	private:
		struct vk_private;

		shared_ptr<vk_private> m_private;

		//ComPtr<IDXGISwapChain1> m_pSwapChain;
		//ComPtr<ID3D11Device> m_pDevice;
		//ComPtr<ID3D11DeviceContext> m_pContext;
		//ComPtr<ID3D11RenderTargetView> m_pBackBuffer;
	};
}