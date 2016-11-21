#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "SkinController.h"
#include "PBREnvironment.h"
#include "TextureHandle.h"
#include "PostProcessItem.h"

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
		unsigned m_AOSamples     = 32;
		float    m_AOOcclusionRadius = 0.13f;
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
		const bb::mat4 getViewProj() const;
		const RendererConfiguration& getConfig() const;

		void resize(unsigned int width, unsigned int height);
		void clear();
		
		void pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group);
		void pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group);
		void pushSkinRenderItem(const SkinRenderItem &skin);
		void pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter);
		void pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::mat4 &transform, const StencilMask filter);
		void pushLight(const bb::vec3 &position, const bb::vec3 &color, const float radius, const float falloff);
		void pushPostProcessItem(const PostProcessItem &proc);

		void setEnvironment(const PBREnvironment &environment);
		void setCamera(const bb::mat4 &view, const bb::mat4 &projection);
		void setConfiguration(const RendererConfiguration &config);

		void render() const;

	private:
		const RenderingContext *m_pRenderContext;

		struct MeshItem
		{
			MeshItem(const RenderMesh &mesh, const float alpha, const bb::mat4 &transform, const StencilMask group)
				: m_Mesh(mesh), m_Alpha(alpha), m_Transform(transform), m_Group(group)
			{}

			RenderMesh    m_Mesh;
			float         m_Alpha;
			bb::mat4      m_Transform;
			StencilMask   m_Group;
		};

		struct DecalItem
		{
			DecalItem(const TextureHandle &texture, const TextureHandle &normal, const bb::mat4 &transform, const StencilMask filter)
				: m_Texture(texture), m_NormalMap(normal), m_Transform(transform), m_Filter(filter) 
			{}

			TextureHandle m_Texture;
			TextureHandle m_NormalMap;
			bb::mat4      m_Transform;
			StencilMask   m_Filter;
		};

		struct PointLightItem
		{
			PointLightItem(const bb::vec3 &position, const bb::vec3 &color, const float radius, const float faloff)
				: m_Position(position), m_Color(color), m_Radius(radius), m_FaloffExponent(faloff) 
			{}

			bb::vec3      m_Position;
			bb::vec3      m_Color;
			float         m_Radius;
			float         m_FaloffExponent;
		};

		//--------------------------------------------------------------------
		// State
		RendererConfiguration             m_Config;
		D3D11_VIEWPORT                    m_ViewPort;
		D3D11_VIEWPORT                    m_BlurViewPort;
		bb::mat4                          m_View;
		bb::mat4                          m_Projection;
		PBREnvironment                    m_Environment;
		vector<MeshItem>                  m_GeometryPositionTexcoord;
		vector<MeshItem>                  m_GeometryPositionNormalTexcoord;
		vector<MeshItem>                  m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<SkinRenderItem>            m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<MeshItem>                  m_GeometryPositionTexcoordTransparent;
		vector<MeshItem>                  m_GeometryPositionNormalTexcoordTransparent;
		vector<MeshItem>                  m_GeometryPositionNormalTangentBinormalTexcoordTransparent;
		vector<SkinRenderItem>            m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent;
		vector<DecalItem>                 m_Decals;
		vector<PointLightItem>            m_PointLights;
		vector<PostProcessItem>           m_PostProcessItems;

		//--------------------------------------------------------------------
		// Private functions
		void renderGeometryToGBuffer() const;
		void renderGBufferToBackBuffer() const;
		template<typename T>
		void renderStaticMeshList(const vector<MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const;
		template<typename T>
		void updateConstantBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, const T &value) const;

		//--------------------------------------------------------------------
		// D3D11 Objects
		
		// GBuffer content:
		static const size_t GBuf_Graphics0Idx    = 0;
		static const size_t GBuf_Graphics1Idx    = 1;
		static const size_t GBuf_Graphics2Idx    = 2;
		static const size_t GBuf_Occlusion0Idx   = 3;
		static const size_t GBuf_Occlusion1Idx   = 4;
		static const size_t GBuf_DepthStencilIdx = 5;
		static const size_t GBuf_ChannelCount    = 6;
		// G-Buffer content:
		// 0:   (albedo.rgb, emissive factor)
		// 1:   (normal.xyz, gloss)
		// 2:   (specular.rgb, cavity)
		// 3-4: (ssao.r)
		// 5:   (depth.r stencil.g)
		ComPtr<ID3D11Texture2D>           m_pGBuffer[GBuf_ChannelCount];
		ComPtr<ID3D11RenderTargetView>    m_pGBufferTarget[GBuf_ChannelCount];
		ComPtr<ID3D11ShaderResourceView>  m_pGBufferView[GBuf_ChannelCount];
		ComPtr<ID3D11DepthStencilView>    m_pDepthBufferView;
		ComPtr<ID3D11DepthStencilView>    m_pDepthBufferViewReadOnly;
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
		ComPtr<ID3D11BlendState>          m_pRenderBlendState;
		ComPtr<ID3D11BlendState>          m_pDefaultBlendState;
		ComPtr<ID3D11SamplerState>        m_pScreenSampler;
		ComPtr<ID3D11Buffer>              m_pScreenQuadBuffer;
		ComPtr<ID3D11ShaderResourceView>  m_pNoiseTexture;
		ComPtr<ID3D11VertexShader>        m_pVSScreenQuad;
		ComPtr<ID3D11InputLayout>         m_pILScreenQuad;
		ComPtr<ID3D11PixelShader>         m_pPSGlobalLighting;
		ComPtr<ID3D11PixelShader>         m_pPSSSAO;
		ComPtr<ID3D11PixelShader>         m_pPSSSAOBlur;
		ComPtr<ID3D11Buffer>              m_pCBSSAO;
		ComPtr<ID3D11Buffer>              m_pSphereVBuffer;
		ComPtr<ID3D11Buffer>              m_pSphereIBuffer;
		ComPtr<ID3D11Buffer>              m_pCubeVBuffer;
		ComPtr<ID3D11Buffer>              m_pCubeIBuffer;
		ComPtr<ID3D11VertexShader>        m_pVSPointLighting;
		ComPtr<ID3D11InputLayout>         m_pILPointLighting;
		ComPtr<ID3D11PixelShader>         m_pPSPointLighting;
		ComPtr<ID3D11Buffer>              m_pCBPointLighting;
		ComPtr<ID3D11RenderTargetView>    m_pPostProcessRT[2];
		ComPtr<ID3D11ShaderResourceView>  m_pPostProcessView[2];
	};
}