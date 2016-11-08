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

	struct RendererConfiguration
	{
		bool     m_AOEnabled     = true;
		unsigned m_AOSamples     = 32;
		float    m_AOOcclusionRadius = 0.13f;
		float    m_AOOcclusionMaxDistance = 1.3f;
		bool     m_AOHiRes       = true;
	};

	class DeferredRenderer
	{
	public:
		DeferredRenderer(const RenderingContext* pRenderContext);

		const RenderingContext* getContext() const;
		const Mat4 getViewProj() const;

		void resize(unsigned int width, unsigned int height);
		void clear();
		
		void pushRenderMesh(const RenderMesh &mesh, const Mat4 &transform, const StencilMask group = 0);
		void pushSkinRenderItem(const SkinRenderItem &skin);
		void pushDecal(const TextureHandle &texture, const Mat4 &transform, const StencilMask filter = 0);
		void pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const Mat4 &transform, const StencilMask filter = 0);
		void pushLight(const Vec3 &position, const Vec3 &color, const float radius, const float falloff);
		void pushPostProcessItem(const PostProcessItem &proc);

		void setEnvironment(const PBREnvironment &environment);
		void setCamera(const Mat4 &view, const Mat4 &projection);
		void setConfiguration(const RendererConfiguration &config);

		void render() const;

	private:
		const RenderingContext *m_pRenderContext;

		struct MeshItem
		{
			MeshItem(const RenderMesh &mesh, const Mat4 &transform, const StencilMask group)
				: m_Mesh(mesh), m_Transform(transform), m_Group(group) 
			{}

			RenderMesh    m_Mesh;
			Mat4          m_Transform;
			StencilMask   m_Group;
		};

		struct DecalItem
		{
			DecalItem(const TextureHandle &texture, const TextureHandle &normal, const Mat4 &transform, const StencilMask filter)
				: m_Texture(texture), m_NormalMap(normal), m_Transform(transform), m_Filter(filter) 
			{}

			TextureHandle m_Texture;
			TextureHandle m_NormalMap;
			Mat4          m_Transform;
			StencilMask   m_Filter;
		};

		struct PointLightItem
		{
			PointLightItem(const Vec3 &position, const Vec3 &color, const float radius, const float faloff)
				: m_Position(position), m_Color(color), m_Radius(radius), m_FaloffExponent(faloff) 
			{}

			Vec3          m_Position;
			Vec3          m_Color;
			float         m_Radius;
			float         m_FaloffExponent;
		};

		//--------------------------------------------------------------------
		// State
		RendererConfiguration             m_Config;
		D3D11_VIEWPORT                    m_ViewPort;
		D3D11_VIEWPORT                    m_BlurViewPort;
		Mat4                              m_View;
		Mat4                              m_Projection;
		PBREnvironment                    m_Environment;
		vector<MeshItem>                  m_GeometryPositionTexcoord;
		vector<MeshItem>                  m_GeometryPositionNormalTexcoord;
		vector<MeshItem>                  m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<SkinRenderItem>            m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<DecalItem>                 m_Decals;
		vector<PointLightItem>            m_PointLights;
		vector<PostProcessItem>           m_PostProcessItems;

		//--------------------------------------------------------------------
		// Private functions
		void renderGeometryToGBuffer() const;
		void renderGBufferToBackBuffer() const;
		template<typename T>
		void renderStaticMeshList(const vector<MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const;

		//--------------------------------------------------------------------
		// D3D11 Objects
		// G-Buffer content:
		// 0:   color buffer
		// 1:   normal buffer
		// 2-3: directional occlusion double buffer
		// 4:   depth buffer
		ComPtr<ID3D11Texture2D>           m_pGBuffer[5];
		ComPtr<ID3D11RenderTargetView>    m_pGBufferTarget[5];
		ComPtr<ID3D11ShaderResourceView>  m_pGBufferView[5];
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
		ComPtr<ID3D11PixelShader>         m_pPSDecals;
		ComPtr<ID3D11SamplerState>        m_pGSampler;
		ComPtr<ID3D11Buffer>              m_pCBScene;
		ComPtr<ID3D11Buffer>              m_pCBObject;
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
		ComPtr<ID3D11PixelShader>         m_pPSDSSDO;
		ComPtr<ID3D11PixelShader>         m_pPSBlur;
		ComPtr<ID3D11Buffer>              m_pCBEffects[2];
		ComPtr<ID3D11Buffer>              m_pCBRandom;
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