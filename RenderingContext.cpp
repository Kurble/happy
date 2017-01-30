#include "stdafx.h"
#include "RenderingContext.h"
#include "GraphicsTimer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <windows.h>
#include <set>

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

	RenderingContext::RenderingContext(HINSTANCE instance, HWND wnd)
	{
		m_private = make_shared<vk_private>();

		m_private->m_VkLayerNames = 
		{ 
			"VK_LAYER_LUNARG_standard_validation",
		};
		
		m_private->filterLayers();

		m_private->m_VkExtensionNames =
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
		if (m_private->m_VkLayerNames.size() > 0) m_private->m_VkExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		
		m_private->filterExtensions();

		m_private->m_VkDeviceExtensionNames =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		m_private->createInstance();

		m_private->createDebugReporter();

		m_private->createSurface(instance, wnd);

		m_private->createPhysicalDevice();

		m_private->createLogicalDevice();

		{
			vk::SemaphoreCreateInfo info;
			m_private->m_ImageReadySignal = m_private->m_Device.createSemaphore(info);
			m_private->m_FrameReadySignal = m_private->m_Device.createSemaphore(info);
		}

		m_private->createSwapChain();

		m_private->createSwapChainImageViews();
	}

	void RenderingContext::acquire()
	{
		auto imageResult = m_private->m_Device.acquireNextImageKHR(m_private->m_SwapChain, UINT64_MAX, m_private->m_ImageReadySignal, VK_NULL_HANDLE);
		switch (imageResult.result)
		{
		case vk::Result::eSuccess:
			break;

		case vk::Result::eSuboptimalKHR:
			break;

		case vk::Result::eErrorOutOfDateKHR:
			m_private->createSwapChain();
			break;
		}
	}

	void RenderingContext::present()
	{
		// todo vk
	}

	void RenderingContext::resize(unsigned int width, unsigned int height)
	{
	}

	unsigned int RenderingContext::getWidth() const
	{
		return m_private->m_Width;
	}

	unsigned int RenderingContext::getHeight() const
	{
		return m_private->m_Height;
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

	struct RenderingContext::vk_private
	{
		std::vector<const char*> m_VkLayerNames;
		std::vector<const char*> m_VkExtensionNames;
		std::vector<const char*> m_VkDeviceExtensionNames;

		vk::SwapchainKHR m_SwapChain;
		vk::Semaphore m_ImageReadySignal;
		vk::Semaphore m_FrameReadySignal;
		vk::Device m_Device;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::SurfaceKHR m_Surface;
		vk::DebugReportCallbackEXT m_DebugReportCB;
		vk::Instance m_Instance;
		vk::Format m_SwapChainFormat;

		vector<vk::Image> m_SwapChainImages;
		vector<vk::Queue> m_Queues;
		uint32_t m_GraphicsQueueIndex;
		uint32_t m_PresentQueueIndex;

		struct QueueFamilyIndices
		{
			int graphics = -1;
			int present = -1;

			bool isCompatible() const
			{
				return graphics >= 0 && present >= 0;
			}
		};

		QueueFamilyIndices m_QueueFamilyIndices;

		void filterLayers()
		{
			auto available = vk::enumerateInstanceLayerProperties();

			m_VkLayerNames.erase(remove_if(m_VkLayerNames.begin(), m_VkLayerNames.end(), [&](const char* layerName)
			{
				bool found = false;
				for (const auto& props : available)
				{
					if (strcmp(layerName, props.layerName) == 0)
					{
						found = true;
						break;
					}
				}

				if (found)
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

					return true;
				}
			}), m_VkLayerNames.end());
		}

		void filterExtensions()
		{
			auto available = vk::enumerateInstanceExtensionProperties();

			m_VkExtensionNames.erase(remove_if(m_VkExtensionNames.begin(), m_VkExtensionNames.end(), [&](const char* layerName)
			{
				bool found = false;
				for (const auto& props : available)
				{
					if (strcmp(layerName, props.extensionName) == 0)
					{
						found = true;
						break;
					}
				}

				if (found)
				{
					stringstream ss;
					ss << "[startup] extension enabled: " << layerName << std::endl;
					OutputDebugStringA(ss.str().c_str());

					return false;
				}
				else
				{
					stringstream ss;
					ss << "[startup] extension disabled: " << layerName << std::endl;
					OutputDebugStringA(ss.str().c_str());

					return true;
				}
			}), m_VkExtensionNames.end());
		}

		void createInstance()
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

				m_Instance = vk::createInstance(info);
				OutputDebugStringA("[startup] created vulkan instance\n");
			}
		}

		void createDebugReporter()
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

		void createSurface(HINSTANCE hInstance, HWND hWnd)
		{
			vk::Win32SurfaceCreateInfoKHR info;
			info.flags = vk::Win32SurfaceCreateFlagsKHR();
			info.hinstance = hInstance;
			info.hwnd = hWnd;

			m_Surface = m_Instance.createWin32SurfaceKHR(info);
		}

		void createPhysicalDevice()
		{
			auto devices = m_Instance.enumeratePhysicalDevices();
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

				bool compatibility = true;
				auto deviceLayers = device.enumerateDeviceLayerProperties();
				auto deviceExtensions = device.enumerateDeviceExtensionProperties();

				for (const char* layerName : m_VkLayerNames)
				{
					bool layerFound = false;
					for (const auto& layerProperties : deviceLayers)
					{
						if (strcmp(layerName, layerProperties.layerName) == 0)
						{
							layerFound = true;
							break;
						}
					}

					if (!layerFound)
					{
						compatibility = false;
						break;
					}
				}

				if (!compatibility) continue;

				for (const char* extensionName : m_VkDeviceExtensionNames)
				{
					bool extensionFound = false;
					for (const auto& extensionProperties : deviceExtensions)
					{
						if (strcmp(extensionName, extensionProperties.extensionName) == 0)
						{
							extensionFound = true;
							break;
						}
					}

					if (!extensionFound)
					{
						compatibility = false;
						break;
					}
				}

				if (!compatibility) continue;

				auto indices = findPhysicalDeviceQueueIndices(&device);
				if (indices.isCompatible())
				{
					unsigned score = 1;
					if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)   score += 100;
					if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 10;
					if (properties.deviceType == vk::PhysicalDeviceType::eOther)         score += 5;
					if (properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu)    score += 2;
					if (properties.deviceType == vk::PhysicalDeviceType::eCpu)           score += 1;

					if (score > best)
					{
						best = score;
						m_PhysicalDevice = device;
						m_QueueFamilyIndices = indices;
					}
				}
			}

			if (!m_PhysicalDevice)
			{
				OutputDebugStringA("[error] no suitable gpu available\n");
				OutputDebugStringA("terminating.. \n");
				std::terminate();
			}
		}

		void createLogicalDevice()
		{
			float queuePriorities[] = { 1.0f };

			set<uint32_t> deviceQueueIndices;
			deviceQueueIndices.insert(m_QueueFamilyIndices.graphics);
			deviceQueueIndices.insert(m_QueueFamilyIndices.present);

			vector<vk::DeviceQueueCreateInfo> queueInfos;
			for (uint32_t queueFamilyIndex : deviceQueueIndices)
			{
				queueInfos.emplace_back();
				queueInfos.back().queueFamilyIndex = queueFamilyIndex;
				queueInfos.back().queueCount = 1;
				queueInfos.back().pQueuePriorities = queuePriorities;

				if (queueFamilyIndex == m_QueueFamilyIndices.graphics) m_GraphicsQueueIndex = queueFamilyIndex;
				if (queueFamilyIndex == m_QueueFamilyIndices.present) m_PresentQueueIndex = queueFamilyIndex;
			}

			vk::PhysicalDeviceFeatures features;
			vk::DeviceCreateInfo info;
			info.pQueueCreateInfos = queueInfos.data();
			info.queueCreateInfoCount = (uint32_t)queueInfos.size();
			info.pEnabledFeatures = &features;
			info.enabledExtensionCount = (uint32_t)m_VkDeviceExtensionNames.size();
			info.ppEnabledExtensionNames = m_VkDeviceExtensionNames.data();
			info.enabledLayerCount = (uint32_t)m_VkLayerNames.size();
			info.ppEnabledLayerNames = m_VkLayerNames.data();

			m_Device = m_PhysicalDevice.createDevice(info);

			for (uint32_t queueFamilyIndex : deviceQueueIndices)
			{
				m_Queues.push_back(m_Device.getQueue(queueFamilyIndex, 0));
			}
		}

		void createSwapChain()
		{
			auto surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
			auto surfaceFormats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
			auto surfacePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);

			uint32_t swapChainLength = min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount);

			vk::SurfaceFormatKHR swapChainFormat{ vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
			for (auto &format : surfaceFormats)
			{
				if (format.format == vk::Format::eR8G8B8A8Unorm)
					swapChainFormat = format;
			}

			vk::Extent2D swapChainSize = surfaceCapabilities.currentExtent;
			if (swapChainSize.width == -1 || swapChainSize.height == -1)
			{
				swapChainSize.width = max(surfaceCapabilities.minImageExtent.width, min(surfaceCapabilities.maxImageExtent.width, 1920));
				swapChainSize.height = max(surfaceCapabilities.minImageExtent.height, min(surfaceCapabilities.maxImageExtent.height, 1080));
			}

			vk::ImageUsageFlags swapChainUsage = vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment);

			if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst)
				swapChainUsage |= vk::ImageUsageFlagBits::eTransferDst;

			vk::SurfaceTransformFlagBitsKHR swapChainTransform = surfaceCapabilities.currentTransform;
			if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
				swapChainTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;

			vk::PresentModeKHR swapChainPresentMode = vk::PresentModeKHR::eFifo;
			for (auto mode : surfacePresentModes)
				if (mode == vk::PresentModeKHR::eMailbox)
					swapChainPresentMode = mode;

			vk::SwapchainCreateInfoKHR info;
			info.surface = m_Surface;
			info.minImageCount = swapChainLength;
			info.imageFormat = swapChainFormat.format;
			info.imageColorSpace = swapChainFormat.colorSpace;
			info.imageExtent = swapChainSize;
			info.imageArrayLayers = 1;
			info.imageUsage = swapChainUsage;
			info.imageSharingMode = vk::SharingMode::eExclusive;
			info.preTransform = swapChainTransform;
			info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			info.presentMode = swapChainPresentMode;
			info.clipped = true;
			info.oldSwapchain = m_SwapChain;

			uint32_t deviceQueueFamilies[] = { (uint32_t)m_QueueFamilyIndices.graphics, (uint32_t)m_QueueFamilyIndices.present };
			if (m_GraphicsQueueIndex != m_PresentQueueIndex)
			{
				info.imageSharingMode = vk::SharingMode::eConcurrent;
				info.queueFamilyIndexCount = 2;
				info.pQueueFamilyIndices = deviceQueueFamilies;
			}

			m_SwapChain = m_Device.createSwapchainKHR(info);

			m_SwapChainImages = m_Device.getSwapchainImagesKHR(m_SwapChain);

			m_SwapChainFormat = vk::Format::eR8G8B8A8Unorm;
		}

		void createSwapChainImageViews()
		{
			// todo
		}

		vk::Queue* getGraphicsQueue()
		{
			return &m_Queues.at(m_GraphicsQueueIndex);
		}

		vk::Queue* getPresentQueue()
		{
			return &m_Queues.at(m_PresentQueueIndex);
		}

		QueueFamilyIndices findPhysicalDeviceQueueIndices(vk::PhysicalDevice* pDevice)
		{
			QueueFamilyIndices indices;

			auto queueFamilies = pDevice->getQueueFamilyProperties();
			int i = 0;

			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueCount > 0)
				{
					if (pDevice->getSurfaceSupportKHR(i, m_Surface))
					{
						indices.present = i;
					}

					if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
					{
						if (indices.graphics == -1) indices.graphics = i;

						if (indices.present == i)
						{
							indices.graphics = i;
							break;
						}
					}
				}

				i++;
			}

			return indices;
		}

		shared_ptr<GraphicsTimer> m_pGraphicsTimer;

		unsigned int m_Width;
		unsigned int m_Height;
	};
}