#pragma once

namespace happy
{
	class PipelineBuilder
	{
	public:
		enum class Topology { PointList, LineList, LineStrip, TriangleList, TriangleStrip };
		enum class PolygonMode { Fill, Line, Point };
		enum class CullMode { Back, Front, All, None };

		PipelineBuilder& ia_topology(Topology topology);
		PipelineBuilder& ia_primitiveRestartEnabled(bool enabled);

		PipelineBuilder& vs_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& ts_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& gs_shader(const char* byteCode, size_t byteCodeLength);
		PipelineBuilder& ps_shader(const char* byteCode, size_t byteCodeLength);

		PipelineBuilder& om_viewport(float x, float y, float width, float height, float minDepth, float maxDepth);
		PipelineBuilder& om_scissor(float x, float y, float width, float height);

		PipelineBuilder& rs_rasterizerDiscardEnabled(bool enabled);
		PipelineBuilder& rs_cullMode(CullMode mode);
		PipelineBuilder& rs_polygonMode(PolygonMode mode);
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
		PipelineBuilder& ds_depthCompareOp();
		PipelineBuilder& ds_depthBoundsTestEnabled(bool enabled);
		PipelineBuilder& ds_stencilTestEnabled(bool enabled);
		PipelineBuilder& ds_frontStencilOp();
		PipelineBuilder& ds_backStencilOp();
		PipelineBuilder& ds_minDepthBounds();
		PipelineBuilder& ds_maxDepthBounds();
	};
}