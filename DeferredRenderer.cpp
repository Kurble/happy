#include "stdafx.h"
#include "DeferredRenderer.h"
#include "DeferredRendererConstBuffers.h"
#include "Sphere.h"
#include "Cube.h"
#include "Resources.h"

#include <algorithm>

#include <vulkan/vulkan.hpp>

namespace happy
{
	struct DeferredRenderer::renderer_private
	{
		const RenderingContext*       m_pRenderContext;
		      RenderTarget*           m_pRenderTarget;
		const vector<PostProcessItem> m_PostProcessing;
		const RendererConfiguration   m_Config;

		renderer_private(
			  const RenderingContext* pRenderContext
			, RenderTarget *pRenderTarget
			, const vector<PostProcessItem>& postProcessing
			, const RendererConfiguration config)
			: m_pRenderContext(pRenderContext)
			, m_pRenderTarget(pRenderTarget)
			, m_PostProcessing(postProcessing)
			, m_Config(config)
		{
			makeRenderPass();
			makeFramebuffers();
		}

		vk::RenderPass          m_RenderPass;
		vector<vk::Framebuffer> m_FramebufferCycle;

		void makeRenderPass()
		{
			vk::Device* device = m_pRenderContext->getDevice();

			//==================================================================================
			// Define the color attachments that will be used in the entire render pass
			vector<vk::AttachmentReference> refs(RenderTarget::gChannelCount);
			for (int i = 0; i < RenderTarget::gChannelCount; ++i)
			{
				refs[i].attachment = i;
				refs[i].layout = (i == RenderTarget::gDepthStencilIdx) ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal;
			}

			vk::AttachmentDescription base;
			base.samples        = vk::SampleCountFlagBits::e1;
			base.loadOp         = vk::AttachmentLoadOp::eClear;
			base.storeOp        = vk::AttachmentStoreOp::eStore;
			base.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
			base.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			base.initialLayout  = vk::ImageLayout::eUndefined;
			base.finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;

			vector<vk::AttachmentDescription> attachments(RenderTarget::gChannelCount, base);
			attachments[RenderTarget::gUVBufferIdx].format          = vk::Format::eR16G16Unorm;
			attachments[RenderTarget::gTangentBufferIdx].format     = vk::Format::eR8G8B8A8Snorm;
			attachments[RenderTarget::gVelocityBufferIdx].format    = vk::Format::eR16G16Sfloat;
			attachments[RenderTarget::gDepthStencilIdx].format      = vk::Format::eD24UnormS8Uint;
			attachments[RenderTarget::gDepthStencilIdx].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			attachments[RenderTarget::gHistory0].initialLayout      = vk::ImageLayout::eColorAttachmentOptimal;
			attachments[RenderTarget::gHistory0].format             = vk::Format::eR8G8B8A8Unorm;
			attachments[RenderTarget::gHistory1].initialLayout      = vk::ImageLayout::eColorAttachmentOptimal;
			attachments[RenderTarget::gHistory1].format             = vk::Format::eR8G8B8A8Unorm;
			attachments[RenderTarget::gHistory1].loadOp             = vk::AttachmentLoadOp::eLoad;
			attachments[RenderTarget::gPostProcess0].initialLayout  = vk::ImageLayout::eColorAttachmentOptimal;
			attachments[RenderTarget::gPostProcess0].format         = vk::Format::eR8G8B8A8Unorm;
			attachments[RenderTarget::gPostProcess0].loadOp         = vk::AttachmentLoadOp::eLoad;
			attachments[RenderTarget::gPostProcess1].initialLayout  = vk::ImageLayout::eColorAttachmentOptimal;
			attachments[RenderTarget::gPostProcess1].format         = vk::Format::eR8G8B8A8Unorm;
			attachments[RenderTarget::gPostProcess1].loadOp         = vk::AttachmentLoadOp::eLoad;

			//==================================================================================
			// Define subpasses
			size_t subpassCount = 2 + m_PostProcessing.size();
			vector<vector<vk::AttachmentReference>> colorAttachmentReferences(subpassCount);
			vector<vector<vk::AttachmentReference>> inputAttachmentReferences(subpassCount);
			vector<vector<uint32_t>> preserveAttachmentReferences(subpassCount);

			vk::SubpassDescription* sp = nullptr; //current working sp

			// subpass 0: geometry pass
			colorAttachmentReferences[0] =
			{
				refs[RenderTarget::gUVBufferIdx],
				refs[RenderTarget::gTangentBufferIdx],
				refs[RenderTarget::gVelocityBufferIdx],
			};

			// subpass 1: deferred sampling, shading and lighting (in one pass/pixel shader)
			colorAttachmentReferences[1] =
			{
				refs[RenderTarget::gHistory0],
			};
			inputAttachmentReferences[1] =
			{
				refs[RenderTarget::gUVBufferIdx],
				refs[RenderTarget::gTangentBufferIdx],
				refs[RenderTarget::gVelocityBufferIdx],
				refs[RenderTarget::gHistory1],
			};

			// post processing subpasses
			size_t subpassIndex = 2;
			for (const auto &postProcess : m_PostProcessing)
			{
				// todo: refactor post processes for vulkan style referencing

				subpassIndex++;
			}

			// compile subpass descriptions
			vector<vk::SubpassDescription> subpasses(subpassCount);
			for (size_t i = 0; i < subpassCount; ++i)
			{
				if (subpasses[i].colorAttachmentCount    = colorAttachmentReferences[i].size())
					subpasses[i].pColorAttachments       = colorAttachmentReferences[i].data();
				if (subpasses[i].inputAttachmentCount    = inputAttachmentReferences[i].size())
					subpasses[i].pInputAttachments       = inputAttachmentReferences[i].data();
				if (subpasses[i].preserveAttachmentCount = preserveAttachmentReferences[i].size())
					subpasses[i].pPreserveAttachments    = preserveAttachmentReferences[i].data();

				subpasses[i].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpasses[i].pDepthStencilAttachment = &refs[RenderTarget::gDepthStencilIdx];
			}

			//==================================================================================
			// Define subpass dependencies
			vector<vk::SubpassDependency> dependencies(1 + m_PostProcessing.size());
			for (size_t i = 0; i < dependencies.size(); ++i)
			{
				dependencies[i].dependencyFlags = vk::DependencyFlagBits::eByRegion;
				dependencies[i].srcSubpass = i;
				dependencies[i].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dependencies[i].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				dependencies[i].dstSubpass = i + 1;
				dependencies[i].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
				dependencies[i].dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
			}

			//==================================================================================
			// Create render pass!
			vk::RenderPassCreateInfo info;
			info.attachmentCount = attachments.size();
			info.pAttachments = attachments.data();
			info.subpassCount = subpasses.size();
			info.pSubpasses = subpasses.data();
			info.dependencyCount = dependencies.size();
			info.pDependencies = dependencies.data();

			m_RenderPass = device->createRenderPass(info);
		}
	
		void makeFramebuffers()
		{
			vk::FramebufferCreateInfo info;
			info.renderPass = m_RenderPass;
			info.attachmentCount = m_pRenderTarget->getAttachmentCount();
			info.width = m_pRenderTarget->getWidth();
			info.height = m_pRenderTarget->getHeight();
			info.layers = 1;

			for (size_t i = 0; i < m_pRenderTarget->getAttachmentPermutationCount(); ++i)
			{
				info.pAttachments = m_pRenderTarget->getAttachmentPermutation(i);
				m_FramebufferCycle.push_back(m_pRenderContext->getDevice()->createFramebuffer(info));
			}
		}
	};

	DeferredRenderer::DeferredRenderer(
		  const RenderingContext* pRenderContext
		, RenderTarget *pRenderTarget
		, const vector<PostProcessItem>& postProcessing
		, const RendererConfiguration config)
	{
		m_private = make_shared<renderer_private>(pRenderContext, pRenderTarget, postProcessing, config);
	}

	const RenderingContext* DeferredRenderer::getContext() const
	{
		return m_private->m_pRenderContext;
	}

	const RendererConfiguration& DeferredRenderer::getConfig() const
	{
		return m_private->m_Config;
	}

	void DeferredRenderer::render(const vector<RenderQueue*>& geometry) const
	{
		// todo
	}
}