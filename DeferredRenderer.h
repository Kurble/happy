#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "SkinController.h"
#include "PBREnvironment.h"
#include "TextureHandle.h"
#include "PostProcessItem.h"
#include "RenderTarget.h"
#include "RenderQueue.h"

namespace happy
{
	using StencilMask = uint8_t;

	enum class Quality
	{
		Low, Normal, High, Extreme,
	};

	struct RendererConfiguration
	{
		unsigned m_AOSamples     = 16;
		float    m_AOOcclusionRadius = 0.4f;
		bool     m_AAEnabled     = true;
		Quality  m_LightingQuality = Quality::Extreme;
		Quality  m_PostEffectQuality = Quality::Extreme;
	};

	class DeferredRenderer
	{
	public:
		DeferredRenderer(
			const RenderingContext* pRenderContext, 
			RenderTarget *pRenderTarget, 
			const vector<PostProcessItem>& postProcessing, 
			const RendererConfiguration config = RendererConfiguration());

		const RenderingContext*      getContext() const;
		const RenderTarget*          getRenderTarget() const;
		const RendererConfiguration& getConfig() const;

		void render(const vector<RenderQueue*>& geometry) const;

	private:
		const RenderingContext *m_pRenderContext;

		struct renderer_private;

		shared_ptr<renderer_private> m_private;
	};
}