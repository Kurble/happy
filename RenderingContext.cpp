#include "stdafx.h"
#include "RenderingContext.h"
#include "GraphicsTimer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

namespace happy
{
	static const bool validationEnabled = true;

	RenderingContext::RenderingContext()
	{
		m_VkLayerNames = 
		{ 
			"VK_LAYER_LUNARG_standard_validation",
		};
		filterLayers();

		m_VkExtensionNames = 
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};
		filterExtensions();

		vk::ApplicationInfo ainfo;
		ainfo.pApplicationName = "";
		ainfo.applicationVersion = 0;
		ainfo.pEngineName = "happy";
		ainfo.engineVersion = 1;
		ainfo.apiVersion = VK_API_VERSION_1_0;

		vk::InstanceCreateInfo iinfo;
		iinfo.pApplicationInfo = &ainfo;
		iinfo.enabledLayerCount = (uint32_t)m_VkLayerNames.size();
		iinfo.ppEnabledLayerNames = m_VkLayerNames.data();

		m_pInstance = make_shared<vk::Instance>(vk::createInstance(iinfo));

		OutputDebugStringA("[startup] created vulkan instance\n");
	}

	void RenderingContext::filterLayers()
	{
		uint32_t layerCount;
		vk::enumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<vk::LayerProperties> availableLayers(layerCount);
		vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		m_VkLayerNames.erase(remove_if(m_VkLayerNames.begin(), m_VkLayerNames.end(), [&](const char* layerName)
		{
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (layerFound)
			{
				stringstream ss;
				ss << "[startup] validation layer enabled: " << layerName << std::endl;
				OutputDebugStringA(ss.str().c_str());

				return false;
			}
			else
			{
				stringstream ss;
				ss << "[startup] validation layer disabled: " << layerName << std::endl;
				OutputDebugStringA(ss.str().c_str());

				return false;
			}
		}), m_VkLayerNames.end());
	}

	void RenderingContext::filterExtensions()
	{
		uint32_t extensionCount;
		vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
		vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		m_VkExtensionNames.erase(remove_if(m_VkExtensionNames.begin(), m_VkExtensionNames.end(), [&](const char* layerName)
		{
			bool extensionFound = false;
			for (const auto& extensionProperties : availableExtensions)
			{
				if (strcmp(layerName, extensionProperties.extensionName) == 0)
				{
					extensionFound = true;
					break;
				}
			}

			if (extensionFound)
			{
				stringstream ss;
				ss << "[startup] extension available: " << layerName << std::endl;
				OutputDebugStringA(ss.str().c_str());

				return false;
			}
			else
			{
				stringstream ss;
				ss << "[error] extension not available: " << layerName << std::endl;
				ss << "terminating.. " << std::endl;
				OutputDebugStringA(ss.str().c_str());

				std::terminate();

				return false;
			}
		}), m_VkExtensionNames.end());
	}

	void RenderingContext::attach(HWND hWnd)
	{
		// todo vk
	}

	void RenderingContext::swap()
	{
		// todo vk
	}

	void RenderingContext::resize(unsigned int width, unsigned int height)
	{
		m_Width = width;
		m_Height = height;

		// todo vk
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
		return nullptr;
	}

	ID3D11DeviceContext* RenderingContext::getContext() const
	{
		return nullptr;
	}

	TimedDeviceContext RenderingContext::getContext(const char *zone) const
	{
		return TimedDeviceContext(nullptr, zone, [] {});
	}

	ID3D11RenderTargetView* RenderingContext::getBackBuffer() const
	{
		return nullptr;
	}
}