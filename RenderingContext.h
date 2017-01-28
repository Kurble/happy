#pragma once

#include "TimedDeviceContext.h"

namespace vk
{
	class Instance;
	class PhysicalDevice;
	class Device;
	class SwapchainKHR;
	class DebugReportCallbackEXT;
	class Queue;
}

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
		shared_ptr<vk::SwapchainKHR> m_pSwapChain;
		shared_ptr<vk::Queue> m_pGraphicsQueue;
		shared_ptr<vk::Device> m_pDevice;
		shared_ptr<vk::PhysicalDevice> m_pPhysicalDevice;
		shared_ptr<vk::DebugReportCallbackEXT> m_pDebugReportCB;
		shared_ptr<vk::Instance> m_pInstance;

		struct QueueFamilyIndices
		{
			int graphics = -1;

			bool isCompatible() const
			{
				return graphics >= 0;
			}
		};
		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<const char*> m_VkLayerNames;
		std::vector<const char*> m_VkExtensionNames;

		void filterLayers();
		void filterExtensions();
		void createInstance();
		void createDebugReporter();
		void createPhysicalDevice();
		void createLogicalDevice();
		QueueFamilyIndices findPhysicalDeviceQueueIndices(vk::PhysicalDevice* pDevice);


		//ComPtr<IDXGISwapChain1> m_pSwapChain;
		//ComPtr<ID3D11Device> m_pDevice;
		//ComPtr<ID3D11DeviceContext> m_pContext;
		//ComPtr<ID3D11RenderTargetView> m_pBackBuffer;

		shared_ptr<GraphicsTimer> m_pGraphicsTimer;

		unsigned int m_Width;
		unsigned int m_Height;
	};
}