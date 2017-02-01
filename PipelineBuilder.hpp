#pragma once

#include <vulkan/vulkan.hpp>

#include "AutoDestroy.hpp"

namespace happy
{
	class PipelineBuilder
	{
	public:
		PipelineBuilder& renderPass(vk::RenderPass pass, unsigned sub, vk::SubpassDescription& subpass)
		{
			m_bsState.attachmentCount = subpass.colorAttachmentCount;

			m_ColorAttachmentCount = subpass.colorAttachmentCount;

			m_info.renderPass = pass;
			m_info.subpass = sub;
			
			return *this;
		}
		PipelineBuilder& shader(vk::ShaderModule module, vk::ShaderStageFlagBits stage)
		{
			// find index for current stage
			size_t index = 0;
			for (auto &shaderStage : m_shaderStages)
			{
				if (shaderStage.stage == stage)
				{
					break;
				}
				index++;
			}

			// create new slot if this stage isn't there yet
			if (index == m_shaderStages.size())
			{
				m_shaderStages.emplace_back();
			}

			// create shader stage
			m_shaderStages[index].module = module;
			m_shaderStages[index].stage = stage;
			m_shaderStages[index].pName = "main";

			return *this;
		}
		PipelineBuilder& device(vk::Device device)                               { m_device = device; return *this; }
		PipelineBuilder& pipelineCache(vk::PipelineCache cache)                  { m_pipelineCache = cache; return *this;}
		PipelineBuilder& ia_topology(vk::PrimitiveTopology topology)             { m_iaState.topology = topology; return *this; }
		PipelineBuilder& ia_primitiveRestartEnabled(bool enabled)                { m_iaState.primitiveRestartEnable = enabled; return *this; }
		PipelineBuilder& vp_viewport(float x, float y, float width, float height, float minDepth, float maxDepth) { m_viewport = { x, y, width, height, minDepth, maxDepth }; return *this; }
		PipelineBuilder& vp_scissor(unsigned x, unsigned y, unsigned width, unsigned height) { m_scissor = { { x, y }, { width, height } }; return *this; }
		PipelineBuilder& rs_rasterizerDiscardEnabled(bool enabled)               { m_rsState.rasterizerDiscardEnable = enabled; return *this; }
		PipelineBuilder& rs_cullMode(vk::CullModeFlags mode)                     { m_rsState.cullMode = mode; return *this; }
		PipelineBuilder& rs_polygonMode(vk::PolygonMode mode)                    { m_rsState.polygonMode = mode; return *this; }
		PipelineBuilder& rs_clockwiseFaces(bool clockwise)                       { m_rsState.frontFace = clockwise ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise; return *this; }
		PipelineBuilder& rs_lineWidth(float width)                               { m_rsState.lineWidth = width; return *this; }
		PipelineBuilder& rs_depthClampEnabled(bool enabled)                      { m_rsState.depthClampEnable = enabled; return *this; }
		PipelineBuilder& rs_depthBiasEnabled(bool enabled)                       { m_rsState.depthBiasEnable = enabled; return *this; }
		PipelineBuilder& rs_depthBiasConstantFactor(float factor)                { m_rsState.depthBiasConstantFactor = factor; return *this; }
		PipelineBuilder& rs_depthBiasClamp(float clamp)                          { m_rsState.depthBiasClamp = clamp; return *this; }
		PipelineBuilder& rs_depthBiasSlopeFactor(float slope)                    { m_rsState.depthBiasSlopeFactor = slope; return *this; }
		PipelineBuilder& ms_sampleShadingEnabled(bool enabled)                   { m_msState.sampleShadingEnable = enabled; return *this; }
		PipelineBuilder& ms_sampleCount(unsigned samples)                        { m_msState.rasterizationSamples = (vk::SampleCountFlagBits)samples; return *this; }
		PipelineBuilder& ms_sampleMinShading(float shading)                      { m_msState.minSampleShading = shading; return *this; }
		PipelineBuilder& ms_sampleMask(unsigned mask)                            { m_sampleMask = mask; return *this; }
		PipelineBuilder& ms_alphaToCoverageEnabled(bool enabled)                 { m_msState.alphaToCoverageEnable = enabled; return *this; }
		PipelineBuilder& ms_alphaToOneEnabled(bool enabled)                      { m_msState.alphaToOneEnable = enabled; return *this; }
		PipelineBuilder& ds_depthTestEnabled(bool enabled)                       { m_dsState.depthTestEnable = enabled; return *this; }
		PipelineBuilder& ds_depthWriteEnabled(bool enabled)                      { m_dsState.depthWriteEnable = enabled; return *this; }
		PipelineBuilder& ds_depthCompareOp(vk::CompareOp compare)                { m_dsState.depthCompareOp = compare; return *this; }
		PipelineBuilder& ds_depthBoundsTestEnabled(bool enabled)                 { m_dsState.depthBoundsTestEnable = enabled; return *this; }
		PipelineBuilder& ds_stencilTestEnabled(bool enabled)                     { m_dsState.stencilTestEnable = enabled; return *this; }
		PipelineBuilder& ds_frontStencilOp(vk::StencilOpState state)             { m_dsState.front = state; return *this; }
		PipelineBuilder& ds_backStencilOp(vk::StencilOpState state)              { m_dsState.back = state; return *this; }
		PipelineBuilder& ds_minDepthBounds(float value)                          { m_dsState.minDepthBounds = value; return *this; }
		PipelineBuilder& ds_maxDepthBounds(float value)                          { m_dsState.maxDepthBounds = value; return *this; }
		PipelineBuilder& bs_colorWriteMask(vk::ColorComponentFlags mask)         { m_bsaState.colorWriteMask = mask; return *this; }
		PipelineBuilder& bs_blendEnabled(bool enabled)                           { m_bsaState.blendEnable = enabled; return *this; }
		PipelineBuilder& bs_srcColorBlendFactor(vk::BlendFactor factor)          { m_bsaState.srcColorBlendFactor = factor; return *this; }
		PipelineBuilder& bs_dstColorBlendFactor(vk::BlendFactor factor)          { m_bsaState.dstColorBlendFactor = factor; return *this; }
		PipelineBuilder& bs_colorBlendOp(vk::BlendOp op)                         { m_bsaState.colorBlendOp = op; return *this; }
		PipelineBuilder& bs_srcAlphaBlendFactor(vk::BlendFactor factor)          { m_bsaState.srcAlphaBlendFactor = factor; return *this; }
		PipelineBuilder& bs_dstAlphaBlendFactor(vk::BlendFactor factor)          { m_bsaState.dstAlphaBlendFactor = factor; return *this; }
		PipelineBuilder& bs_alphaBlendOp(vk::BlendOp op)                         { m_bsaState.alphaBlendOp = op; return *this; }
		PipelineBuilder& bs_logicOpEnabled(bool enabled)                         { m_bsState.logicOpEnable = enabled; return *this; }
		PipelineBuilder& bs_logicOp(vk::LogicOp op)                              { m_bsState.logicOp = op; return *this; }
		PipelineBuilder& bs_blendConstant(float r, float g, float b, float a)    { m_bsState.blendConstants[0] = r; m_bsState.blendConstants[1] = g; m_bsState.blendConstants[2] = b; m_bsState.blendConstants[3] = a; }

		vk::Pipeline build()
		{
			m_vpState.viewportCount = 1;
			m_vpState.pViewports = &m_viewport;
			m_vpState.scissorCount = 1;
			m_vpState.pScissors = &m_scissor;

			m_msState.pSampleMask = &m_sampleMask;

			std::vector<vk::PipelineColorBlendAttachmentState> attachments;
			attachments.resize(m_bsState.attachmentCount, m_bsaState);
			m_bsState.pAttachments = attachments.data();

			m_shaderStages.reserve(4);

			m_info.stageCount = 0;
			m_info.pStages = m_shaderStages.data();
			m_info.pVertexInputState = &m_inputState;
			m_info.pInputAssemblyState = &m_iaState;
			m_info.pTessellationState = &m_tsState;
			m_info.pViewportState = &m_vpState;
			m_info.pRasterizationState = &m_rsState;
			m_info.pMultisampleState = &m_msState;
			m_info.pDepthStencilState = &m_dsState;
			m_info.pColorBlendState = &m_bsState;
			m_info.pDynamicState = nullptr;

			return m_device.createGraphicsPipeline(m_pipelineCache, m_info);
		}

	private:
		vk::Device                                     m_device;
		vk::PipelineCache                              m_pipelineCache;

		std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

		vk::PipelineVertexInputStateCreateInfo         m_inputState;

		vk::Viewport                                   m_viewport;
		vk::Rect2D                                     m_scissor;
		vk::SampleMask                                 m_sampleMask;
		unsigned                                       m_ColorAttachmentCount;
												       
		vk::PipelineInputAssemblyStateCreateInfo       m_iaState;
		vk::PipelineTessellationStateCreateInfo        m_tsState;
		vk::PipelineViewportStateCreateInfo            m_vpState;
		vk::PipelineRasterizationStateCreateInfo       m_rsState;
		vk::PipelineMultisampleStateCreateInfo         m_msState;
		vk::PipelineDepthStencilStateCreateInfo        m_dsState;
		vk::PipelineColorBlendAttachmentState          m_bsaState;
		vk::PipelineColorBlendStateCreateInfo          m_bsState;
												       
		vk::GraphicsPipelineCreateInfo                 m_info;
	};
}