#include "stdafx.h"
#include "RenderingContext.h"
#include "GraphicsTimer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <windows.h>

namespace happy
{
	static const bool validationEnabled = true;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		  VkDebugReportFlagsEXT flags
		, VkDebugReportObjectTypeEXT objType
		, uint64_t obj
		, size_t location
		, int32_t code
		, const char* layerPrefix
		, const char* msg
		, void* userData) 
	{
		stringstream ss;

		ss << "[debug] validation layer: " << msg << std::endl;
		
		OutputDebugStringA(ss.str().c_str());

		return VK_FALSE;
	}

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
		if (m_VkLayerNames.size() > 0) m_VkExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

		filterExtensions();

		createInstance();

		createDebugReporter();

		createPhysicalDevice();

		createLogicalDevice();
	}

	void RenderingContext::filterLayers()
	{
		auto availableLayers = vk::enumerateInstanceLayerProperties();

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
		auto availableExtensions = vk::enumerateInstanceExtensionProperties();

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

	void RenderingContext::createInstance()
	{
		vk::ApplicationInfo ainfo;
		ainfo.pApplicationName = "";
		ainfo.applicationVersion = 0;
		ainfo.pEngineName = "happy";
		ainfo.engineVersion = 1;
		ainfo.apiVersion = VK_API_VERSION_1_0;

		// vk instance creation
		{
			vk::InstanceCreateInfo info;
			info.pApplicationInfo = &ainfo;
			info.enabledLayerCount = (uint32_t)m_VkLayerNames.size();
			info.ppEnabledLayerNames = m_VkLayerNames.data();
			info.enabledExtensionCount = (uint32_t)m_VkExtensionNames.size();
			info.ppEnabledExtensionNames = m_VkExtensionNames.data();

			m_pInstance = make_shared<vk::Instance>(vk::createInstance(info));
			OutputDebugStringA("[startup] created vulkan instance\n");
		}
	}

	void RenderingContext::createDebugReporter()
	{
		if (m_VkLayerNames.size() > 0)
		{
			vk::DebugReportCallbackCreateInfoEXT info;
			info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
			info.pfnCallback = debugCallback;

			//m_pInstance->createDebugReportCallbackEXT(info);
			//vkCreateDebugReportCallbackEXT
		}
	}

	void RenderingContext::createPhysicalDevice()
	{
		auto devices = m_pInstance->enumeratePhysicalDevices();
		if (devices.size() == 0)
		{
			OutputDebugStringA("[error] no vulkan capable gpu available\n");
			OutputDebugStringA("terminating.. \n");
			std::terminate();
		}

		unsigned best = 0;

		for (auto device : devices)
		{
			auto properties = device.getProperties();
			
			auto indices = findPhysicalDeviceQueueIndices(&device);
			if (indices.isCompatible())
			{
				unsigned score = 1;
				if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 100;
				if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 10;
				if (properties.deviceType == vk::PhysicalDeviceType::eOther) score += 5;
				if (properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu) score += 2;
				if (properties.deviceType == vk::PhysicalDeviceType::eCpu) score += 1;

				if (score > best)
				{
					best = score;
					m_pPhysicalDevice = make_shared<vk::PhysicalDevice>(device);
					m_QueueFamilyIndices = indices;
				}
			}
		}

		if (!m_pPhysicalDevice)
		{
			OutputDebugStringA("[error] no suitable gpu available\n");
			OutputDebugStringA("terminating.. \n");
			std::terminate();
		}
	}

	RenderingContext::QueueFamilyIndices RenderingContext::findPhysicalDeviceQueueIndices(vk::PhysicalDevice* pDevice)
	{
		QueueFamilyIndices indices;
		
		auto queueFamilies = pDevice->getQueueFamilyProperties();
		int i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0)
			{
				if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) indices.graphics = i;
			}

			i++;
		}

		return indices;
	}

	void RenderingContext::createLogicalDevice()
	{
		float grahpicsQueuePriorities[] = { 1.0f };

		vector<vk::DeviceQueueCreateInfo> queueInfos(1);
		queueInfos[0].queueFamilyIndex = m_QueueFamilyIndices.graphics;
		queueInfos[0].queueCount = 1;
		queueInfos[0].pQueuePriorities = grahpicsQueuePriorities;

		vk::PhysicalDeviceFeatures features;
		vk::DeviceCreateInfo info;
		info.pQueueCreateInfos = queueInfos.data();
		info.queueCreateInfoCount = (uint32_t)queueInfos.size();
		info.pEnabledFeatures = &features;
		info.enabledExtensionCount = 0;
		info.enabledLayerCount = m_VkLayerNames.size();
		info.ppEnabledLayerNames = m_VkLayerNames.data();

		m_pDevice = make_shared<vk::Device>(m_pPhysicalDevice->createDevice(info));
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