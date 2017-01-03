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
		DeferredRenderer(const RenderingContext* pRenderContext, const RendererConfiguration config = RendererConfiguration());

		const RenderingContext* getContext() const;
		const RendererConfiguration& getConfig() const;

		void render(const RenderQueue *scene, RenderTarget *target) const;

	private:
		const RenderingContext *m_pRenderContext;

		//=========================================================
		// Private methods
		//=========================================================
		void createStates(const RenderingContext *pRenderContext);
		void createGeometries(const RenderingContext *pRenderContext);
		void createBuffers(const RenderingContext *pRenderContext);
		void createShaders(const RenderingContext *pRenderContext);
		void renderGeometry(const RenderQueue *scene, RenderTarget *target) const;
		void renderStaticMeshList(const vector<RenderQueue::MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const;
		void renderSkinList(const vector<SkinRenderItem> &renderList) const;
		void renderLighting(const RenderQueue *scene, RenderTarget *target) const;
		void renderScreenSpacePass(ID3D11PixelShader *ps, ID3D11RenderTargetView* rtv, ID3D11Buffer** constBuffers, ID3D11ShaderResourceView** srvs, ID3D11SamplerState** samplers) const;
		void renderPostProcessing(const RenderQueue *scene, RenderTarget *target) const;

		//=========================================================
		// State
		//=========================================================
		RendererConfiguration             m_Config;
		PostProcessItem                   m_ColorGrading;

		//=========================================================
		// D3D11 State Objects
		//=========================================================
		ComPtr<ID3D11RasterizerState>     m_pRasterState;
		ComPtr<ID3D11InputLayout>         m_pILPositionTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTangentBinormalTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTangentBinormalTexcoordIndicesWeights;
		ComPtr<ID3D11VertexShader>        m_pVSPositionTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTangentBinormalTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights;
		ComPtr<ID3D11PixelShader>         m_pPSGeometry;
		ComPtr<ID3D11PixelShader>         m_pPSGeometryAlphaStippled;
		ComPtr<ID3D11PixelShader>         m_pPSDecals;
		ComPtr<ID3D11SamplerState>        m_pGSampler;
		ComPtr<ID3D11Buffer>              m_pCBScene;
		ComPtr<ID3D11Buffer>              m_pCBObject;
		ComPtr<ID3D11Buffer>              m_pCBSkin;
		ComPtr<ID3D11DepthStencilState>   m_pGBufferDepthStencilState;
		ComPtr<ID3D11DepthStencilState>   m_pLightingDepthStencilState;
		ComPtr<ID3D11DepthStencilState>   m_pDecalsDepthStencilState;
		ComPtr<ID3D11BlendState>          m_pDecalBlendState;
		ComPtr<ID3D11SamplerState>        m_pScreenSampler;
		ComPtr<ID3D11Buffer>              m_pScreenQuadBuffer;
		ComPtr<ID3D11ShaderResourceView>  m_pNoiseTexture[RenderTarget::MultiSamples];
		ComPtr<ID3D11VertexShader>        m_pVSScreenQuad;
		ComPtr<ID3D11InputLayout>         m_pILScreenQuad;
		ComPtr<ID3D11PixelShader>         m_pPSGlobalLighting;
		ComPtr<ID3D11PixelShader>         m_pPSSSAO;
		ComPtr<ID3D11Buffer>              m_pCBSSAO;
		ComPtr<ID3D11PixelShader>         m_pPSTAA;
		ComPtr<ID3D11Buffer>              m_pCBTAA;
		ComPtr<ID3D11Buffer>              m_pSphereVBuffer;
		ComPtr<ID3D11Buffer>              m_pSphereIBuffer;
		ComPtr<ID3D11Buffer>              m_pCubeVBuffer;
		ComPtr<ID3D11Buffer>              m_pCubeIBuffer;
		ComPtr<ID3D11VertexShader>        m_pVSPointLighting;
		ComPtr<ID3D11InputLayout>         m_pILPointLighting;
		ComPtr<ID3D11PixelShader>         m_pPSPointLighting;
		ComPtr<ID3D11Buffer>              m_pCBPointLighting;
	};
}