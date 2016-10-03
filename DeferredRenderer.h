#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "SkinController.h"
#include "PBREnvironment.h"
#include "TextureHandle.h"
#include "PostProcessItem.h"

namespace happy
{
	using RenderList = vector<pair<RenderMesh, Mat4>>;

	struct RendererConfiguration
	{
		bool     m_AOEnabled     = true;
		unsigned m_AOSamples     = 32;
		float    m_AOOcclusionRadius = 0.13f;
		float    m_AOOcclusionMaxDistance = 1.3f;
		bool     m_AOHiRes       = true;
	};

	struct CBufferScene
	{
		Mat4 view;
		Mat4 projection;
		Mat4 viewInverse;
		Mat4 projectionInverse;
		float width;
		float height;
		unsigned int convolutionStages;
		unsigned int aoEnabled;
	};

	struct CBufferObject
	{
		Mat4 world;
		float blendAnim[4];
		float blendFrame[4];
		unsigned int animationCount;
	};

	struct CBufferPointLight
	{
		Vec4 position;
		Vec4 color;
		float scale;
		float falloff;
	};

	struct CBufferEffects
	{
		float occlusionRadius = 0.1f;
		float occlusionMaxDistance = 10.2f;
		Vec2  blurDir;
		
		int   samples = 128;
	};

	struct CBufferRandom
	{
		Vec2 random_points[512];
	};

	class DeferredRenderer
	{
	public:
		DeferredRenderer(const RenderingContext* pRenderContext);

		const RenderingContext* getContext() const;

		void resize(unsigned int width, unsigned int height);
		void clear();
		
		void pushRenderMesh(const RenderMesh &mesh, const Mat4 &transform);
		void pushSkinRenderItem(const SkinRenderItem &skin);
		void pushLight(const Vec3 &position, const Vec3 &color, const float radius, const float falloff);
		void pushPostProcessItem(const PostProcessItem &proc);

		void setEnvironment(const PBREnvironment &environment);
		void setCamera(const Mat4 &view, const Mat4 &projection);
		void setConfiguration(const RendererConfiguration &config);

		void render() const;

	private:
		const RenderingContext *m_pRenderContext;

		RendererConfiguration m_Config;

		void renderGeometryToGBuffer() const;
		void renderGBufferToBackBuffer() const;

		struct PointLight
		{
			Vec3 m_Position;
			Vec3 m_Color;
			float m_Radius;
			float m_FaloffExponent;
		};

		ComPtr<ID3D11RasterizerState>     m_pRasterState;

		//--------------------------------------------------------------------
		// G-Buffer shaders, static
		ComPtr<ID3D11InputLayout>         m_pILPositionTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTangentBinormalTexcoord;
		ComPtr<ID3D11InputLayout>         m_pILPositionNormalTangentBinormalTexcoordIndicesWeights;
		ComPtr<ID3D11VertexShader>        m_pVSPositionTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTangentBinormalTexcoord;
		ComPtr<ID3D11VertexShader>        m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights;
		ComPtr<ID3D11PixelShader>         m_pPSGeometry;
		ComPtr<ID3D11SamplerState>        m_pGSampler;
		ComPtr<ID3D11Buffer>              m_pCBScene;
		ComPtr<ID3D11Buffer>              m_pCBObject;

		//--------------------------------------------------------------------
		// G-Buffer content:
		// 0:   color buffer
		// 1:   normal buffer
		// 2-3: directional occlusion double buffer
		// 4:   depth buffer
		ComPtr<ID3D11Texture2D>           m_pGBuffer[5];
		ComPtr<ID3D11RenderTargetView>    m_pGBufferTarget[5];
		ComPtr<ID3D11ShaderResourceView>  m_pGBufferView[5];
		ComPtr<ID3D11DepthStencilView>    m_pDepthBufferView;

		//--------------------------------------------------------------------
		// Screen rendering
		ComPtr<ID3D11DepthStencilState>   m_pRenderDepthState;
		ComPtr<ID3D11BlendState>          m_pRenderBlendState;
		ComPtr<ID3D11BlendState>          m_pDefaultBlendState;
		ComPtr<ID3D11SamplerState>        m_pScreenSampler;
		ComPtr<ID3D11Buffer>              m_pScreenQuadBuffer;
		ComPtr<ID3D11ShaderResourceView>  m_pNoiseTexture;
		ComPtr<ID3D11VertexShader>        m_pVSScreenQuad;
		ComPtr<ID3D11InputLayout>         m_pILScreenQuad;
		ComPtr<ID3D11PixelShader>         m_pPSGlobalLighting;
		ComPtr<ID3D11PixelShader>         m_pPSDSSDO;
		ComPtr<ID3D11PixelShader>         m_pPSBlur;
		ComPtr<ID3D11Buffer>              m_pCBEffects[2];
		ComPtr<ID3D11Buffer>              m_pCBRandom;
		ComPtr<ID3D11Buffer>              m_pSphereVBuffer;
		ComPtr<ID3D11Buffer>              m_pSphereIBuffer;
		ComPtr<ID3D11VertexShader>        m_pVSPointLighting;
		ComPtr<ID3D11InputLayout>         m_pILPointLighting;
		ComPtr<ID3D11PixelShader>         m_pPSPointLighting;
		ComPtr<ID3D11Buffer>              m_pCBPointLighting;
		ComPtr<ID3D11RenderTargetView>    m_pPostProcessRT[2];
		ComPtr<ID3D11ShaderResourceView>  m_pPostProcessView[2];

		//--------------------------------------------------------------------
		// State
		D3D11_VIEWPORT                    m_ViewPort;
		D3D11_VIEWPORT                    m_BlurViewPort;
		Mat4                              m_View;
		Mat4                              m_Projection;
		PBREnvironment                    m_Environment;
		RenderList                        m_GeometryPositionTexcoord;
		RenderList                        m_GeometryPositionNormalTexcoord;
		RenderList                        m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<SkinRenderItem>            m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<PointLight>                m_PointLights;
		vector<PostProcessItem>           m_PostProcessItems;
	};
}