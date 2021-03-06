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

		void                    setView(bb::mat4 &view);
		void                    setProjection(bb::mat4 &projection);
		void                    setViewport(array<float, 4> &viewport);
		void                    setOutput(ID3D11RenderTargetView* target);

	private:
		friend class DeferredRenderer;

		ID3D11RenderTargetView* historyRTV() const;
		ID3D11ShaderResourceView* historySRV() const;
		ID3D11ShaderResourceView* currentSRV() const;

		static const size_t GBuf_Graphics0Idx    = 0; // albedo.rgb, emissive
		static const size_t GBuf_Graphics1Idx    = 1; // normal.xyz
		static const size_t GBuf_Graphics2Idx    = 2; // specular.rgb, gloss
		static const size_t GBuf_VelocityIdx     = 3; // velocity.xy
		static const size_t GBuf_OcclusionIdx    = 4; // ssao
		static const size_t GBuf_DepthStencilIdx = 5; // depth, stencil

		static const size_t GBuf_ChannelCount    = 6;

		static const size_t MultiSamples = 8;

		RenderingContext* m_pRenderContext;

		struct TargetPair
		{
			ComPtr<ID3D11RenderTargetView>   rtv;
			ComPtr<ID3D11ShaderResourceView> srv;
		};

		array<float, 4> m_ViewPortArray;
		D3D11_VIEWPORT m_ViewPort;
		D3D11_VIEWPORT m_BlurViewPort;
		bb::mat4 m_View;
		bb::mat4 m_Projection;
		bb::mat4 m_ViewHistory;
		bb::mat4 m_ProjectionHistory;

		unsigned m_JitterIndex = 0;
		bb::vec2 m_Jitter[MultiSamples];

		ID3D11RenderTargetView* m_pOutputTarget;

		TargetPair m_GraphicsBuffer[GBuf_ChannelCount];
		TargetPair m_HistoryBuffer[2];
		TargetPair m_PostBuffer[2];
		ComPtr<ID3D11DepthStencilView> m_pDepthBufferView;
		ComPtr<ID3D11DepthStencilView> m_pDepthBufferViewReadOnly;

		unsigned m_LastUsedHistoryBuffer = 0;
	};
}
