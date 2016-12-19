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
		bool     m_AOEnabled     = true;
		unsigned m_AOSamples     = 16;
		float    m_AOOcclusionRadius = 0.4f;
		bool     m_AOHiRes       = true;
		
				/* extreme: parallax occlusion *
				/* high:    pbr + ao maps */
				/* normal:  roughness mapping */
				/* low:     only N dot L */
		Quality m_LightingQuality = Quality::Extreme;

				/* extreme: dssdo *
				/* high:    no post effects */
				/* normal:  no post effects */
				/* low:     no post effects */
		Quality m_PostEffectQuality = Quality::Extreme;
	};

	class DeferredRenderer
	{
	public:
		DeferredRenderer(const RenderingContext* pRenderContext, const RendererConfiguration config = RendererConfiguration());

		const RenderingContext* getContext() const;
		const RendererConfiguration& getConfig() const;

		void setConfiguration(const RendererConfiguration &config);

		void render(const RenderQueue *scene, RenderTarget *target) const;

	private:
		const RenderingContext *m_pRenderContext;

		//--------------------------------------------------------------------
		// Private functions
		void createStates(const RenderingContext *pRenderContext);
		void createGeometries(const RenderingContext *pRenderContext);
		void createBuffers(const RenderingContext *pRenderContext);
		void createShaders(const RenderingContext *pRenderContext);

		void renderGeometry(const RenderQueue *scene, RenderTarget *target) const;
		void renderDeferred(const RenderQueue *scene, RenderTarget *target) const;
		template<typename T>
		void renderStaticMeshList(const vector<RenderQueue::MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const;
		template<typename T>
		void updateConstantBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, const T &value) const;

		//--------------------------------------------------------------------
		// State
		RendererConfiguration             m_Config;

		//--------------------------------------------------------------------
		// D3D11 State Objects
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
		ComPtr<ID3D11PixelShader>         m_pPSSSAOBlur;
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