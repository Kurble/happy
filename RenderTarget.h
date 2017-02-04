#pragma once

#include "TextureHandle.h"
#include "RenderingContext.h"

#include "bb_lib\vec2.h"
#include "bb_lib\vec3.h"
#include "bb_lib\vec4.h"
#include "bb_lib\mat3.h"
#include "bb_lib\mat4.h"
#include "bb_lib\math_util.h"

#include <array>

namespace vk
{
	class ImageView;
}

namespace happy
{
	class RenderTarget : public TextureHandle
	{
	public:
		RenderTarget(RenderingContext *context, unsigned width, unsigned height, bool hiresEffects);

		void resize(unsigned width, unsigned height, bool hiresEffects);

		const RenderingContext* getContext() const;
		const bb::mat4&         getView() const;
		const bb::mat4&         getProjection() const;
		const float             getWidth() const;
		const float             getHeight() const;
		const array<float, 4>&  getViewport() const;

		const vk::ImageView*    getOddAttachments() const;
		const vk::ImageView*    getEvenAttachments() const;

		void                    setView(bb::mat4 &view);
		void                    setProjection(bb::mat4 &projection);
		void                    setViewport(bb::vec2 offset, bb::vec2 size);

		static const size_t     gUVBufferIdx       = 0;
		static const size_t     gTangentBufferIdx  = 1;
		static const size_t     gVelocityBufferIdx = 2;
		static const size_t     gDepthStencilIdx   = 3;
		static const size_t     gHistory0          = 4;
		static const size_t     gHistory1          = 5;
		static const size_t     gPostProcess0      = 6;
		static const size_t     gPostProcess1      = 7;
						       
		static const size_t     gChannelCount      = 8;
						       
		static const size_t     MultiSamples       = 8;

	private:
		friend class  DeferredRenderer;

		RenderingContext* m_pRenderContext;

		uint32_t m_LastUsedHistoryBuffer = 0;

		struct rt_private;

		shared_ptr<rt_private> m_private;
	};
}
