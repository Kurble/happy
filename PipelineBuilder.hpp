#pragma once

#include <vulkan/vulkan.hpp>

namespace happy
{
	class PipelineBuilder
	{
	public:
		PipelineBuilder& ia_topology(vk::PrimitiveTopology topology);
		PipelineBuilder& ia_primitiveRestartEnabled(bool enabled);

		PipelineBuilder& vs_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& ts_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& gs_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& ps_shader(const char* byteCode, size_t byteCodeLength);

		PipelineBuilder& vp_viewport(float x, float y, float width, float height, float minDepth, float maxDepth);
		PipelineBuilder& vp_scissor(float x, float y, float width, float height);

		PipelineBuilder& rs_rasterizerDiscardEnabled(bool enabled);
		PipelineBuilder& rs_cullMode(vk::CullModeFlags mode);
		PipelineBuilder& rs_polygonMode(vk::PolygonMode mode);
		PipelineBuilder& rs_clockwiseFaces(bool clockwise);
		PipelineBuilder& rs_lineWidth(float width);
		PipelineBuilder& rs_depthClampEnabled(bool enabled);
		PipelineBuilder& rs_depthBiasEnabled(bool enabled);
		PipelineBuilder& rs_depthBiasConstantFactor(float factor);
		PipelineBuilder& rs_depthBiasClamp(float clamp);
		PipelineBuilder& rs_depthBiasSlopeFactor(float slope);

		PipelineBuilder& ms_sampleShadingEnable(bool enable);
		PipelineBuilder& ms_sampleCount(unsigned samples);
		PipelineBuilder& ms_sampleMinShading(float shading);
		PipelineBuilder& ms_sampleMask(unsigned mask);
		PipelineBuilder& ms_alphaToCoverageEnabled(bool enabled);
		PipelineBuilder& ms_alphaToOneEnabled(bool enabled);

		PipelineBuilder& ds_depthTestEnabled(bool enabled);
		PipelineBuilder& ds_depthWriteEnabled(bool enabled);
		PipelineBuilder& ds_depthCompareOp(vk::CompareOp compare);
		PipelineBuilder& ds_depthBoundsTestEnabled(bool enabled);
		PipelineBuilder& ds_stencilTestEnabled(bool enabled);
		PipelineBuilder& ds_frontStencilOp(vk::StencilOpState state);
		PipelineBuilder& ds_backStencilOp(vk::StencilOpState state);
		PipelineBuilder& ds_minDepthBounds(float value);
		PipelineBuilder& ds_maxDepthBounds(float value);

		PipelineBuilder& bs_colorWriteMask(vk::ColorComponentFlags mask);
		PipelineBuilder& bs_blendEnabled(bool enabled);
		PipelineBuilder& bs_srcColorBlendFactor(vk::BlendFactor factor);
		PipelineBuilder& bs_dstColorBlendFactor(vk::BlendFactor factor);
		PipelineBuilder& bs_colorBlendOp(vk::BlendOp op);
		PipelineBuilder& bs_srcAlphaBlendFactor(vk::BlendFactor factor);
		PipelineBuilder& bs_dstAlphaBlendFactor(vk::BlendFactor factor);
		PipelineBuilder& bs_alphaBlendOp(vk::BlendOp op);
		PipelineBuilder& bs_logicOpEnabled(bool enabled);
		PipelineBuilder& bs_logicOp(vk::LogicOp op);
		PipelineBuilder& bs_blendConstant(float r, float g, float b, float a);

		vk::Pipeline build();

	private:
		vk::ShaderModule                         m_shaderModules[4];
		vk::PipelineShaderStageCreateInfo        m_shaderStages[4];

		vk::PipelineVertexInputStateCreateInfo   m_inputState;

		vk::PipelineInputAssemblyStateCreateInfo m_iaState;
		vk::PipelineViewportStateCreateInfo      m_vpState;
		vk::PipelineRasterizationStateCreateInfo m_rsState;
		vk::PipelineMultisampleStateCreateInfo   m_msState;
		vk::PipelineDepthStencilStateCreateInfo  m_dsState;
		vk::PipelineColorBlendAttachmentState    m_bsState;

		vk::GraphicsPipelineCreateInfo           m_info;
	};
}