#include "stdafx.h"
#include "RenderTarget.h"

#include "bb_lib\halton.h"

#include <vulkan/vulkan.hpp>

namespace happy
{
	struct RenderTarget::rt_private 
	{
		vk::RenderPass m_RenderPass;
		
		bb::vec2 m_ViewportOffset;
		bb::vec2 m_ViewportSize;

		bb::mat4 m_View;
		bb::mat4 m_Projection;
		bb::mat4 m_ViewHistory;
		bb::mat4 m_ProjectionHistory;

		unsigned m_JitterIndex = 0;
		bb::vec2 m_Jitter[MultiSamples];

		unsigned m_LastUsedHistoryBuffer = 0;

		
	};

	RenderTarget::RenderTarget(RenderingContext *context, unsigned width, unsigned height, bool hiresEffects)
	{
		m_private = make_shared<rt_private>();
	}

	void RenderTarget::resize(unsigned width, unsigned height, bool hiresEffects)
	{
		//
	}

	const RenderingContext* RenderTarget::getContext() const
	{
		return m_pRenderContext;
	}

	const bb::mat4& RenderTarget::getView() const
	{
		return m_private->m_View;
	}

	const bb::mat4& RenderTarget::getProjection() const
	{
		return m_private->m_Projection;
	}
	
	const float RenderTarget::getWidth() const
	{
		return m_private->m_ViewportSize.x;
	}

	const float RenderTarget::getHeight() const
	{
		return m_private->m_ViewportSize.y;
	}

	void RenderTarget::setViewport(bb::vec2 offset, bb::vec2 size)
	{
		m_private->m_ViewportOffset = offset;
		m_private->m_ViewportSize = size;
	}

	void RenderTarget::setView(bb::mat4 &view)
	{
		m_private->m_View = view;
	}

	void RenderTarget::setProjection(bb::mat4 &projection)
	{
		m_private->m_Projection = projection;
	}

	void RenderTarget::setOutputToFramebuffer(vk::Framebuffer* framebuffer)
	{
		//
	}
}