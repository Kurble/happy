#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "SkinController.h"
#include "PBREnvironment.h"

namespace happy
{
	using RenderList = vector<pair<RenderMesh, Mat4>>;

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

		void setEnvironment(const PBREnvironment &environment);
		void setCamera(const Mat4 &view, const Mat4 &projection);

		void render() const;

	private:
		const RenderingContext *m_pRenderContext;

		void renderGeometryToGBuffer() const;
		void renderGBufferToBackBuffer() const;

		template <typename T, size_t Length> void CreateVertexShader(ComPtr<ID3D11VertexShader> &vs, ComPtr<ID3D11InputLayout> &il, const BYTE (&shaderByteCode)[Length])
		{
			size_t length = Length;
			THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateVertexShader(shaderByteCode, length, nullptr, &vs));
			THROW_ON_FAIL(m_pRenderContext->getDevice()->CreateInputLayout(T::Elements, T::ElementCount, shaderByteCode, length, &il));
		}

		template <size_t Length> void CreatePixelShader(ComPtr<ID3D11PixelShader> &ps, const BYTE(&shaderByteCode)[Length])
		{
			size_t length = Length;
			THROW_ON_FAIL(m_pRenderContext->getDevice()->CreatePixelShader(shaderByteCode, length, nullptr, &ps));
		}

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
		// G-Buffer shaders, skinned
		/* sorry no shaders yet */

		//--------------------------------------------------------------------
		// G-Buffer content:
		// 0: color buffer
		// 1: normal buffer
		// 2: directional occlusion buffer
		// 3: depth buffer
		ComPtr<ID3D11Texture2D>           m_pGBuffer[4];
		ComPtr<ID3D11RenderTargetView>    m_pGBufferTarget[4];
		ComPtr<ID3D11ShaderResourceView>  m_pGBufferView[4];
		ComPtr<ID3D11DepthStencilView>    m_pDepthBufferView;

		//--------------------------------------------------------------------
		// Screen rendering
		ComPtr<ID3D11DepthStencilState>   m_pRenderDepthState;
		ComPtr<ID3D11BlendState>          m_pRenderBlendState;
		ComPtr<ID3D11SamplerState>        m_pScreenSampler;
		ComPtr<ID3D11Buffer>              m_pScreenQuadBuffer;
		ComPtr<ID3D11ShaderResourceView>  m_pNoiseTexture;
		ComPtr<ID3D11VertexShader>        m_pVSScreenQuad;
		ComPtr<ID3D11InputLayout>         m_pILScreenQuad;
		ComPtr<ID3D11PixelShader>         m_pPSGlobalLighting;
		ComPtr<ID3D11PixelShader>         m_pPSDSSDO;
		ComPtr<ID3D11Buffer>              m_pSphereVBuffer;
		ComPtr<ID3D11Buffer>              m_pSphereIBuffer;
		ComPtr<ID3D11VertexShader>        m_pVSPointLighting;
		ComPtr<ID3D11InputLayout>         m_pILPointLighting;
		ComPtr<ID3D11PixelShader>         m_pPSPointLighting;
		ComPtr<ID3D11Buffer>              m_pCBPointLighting;


		//--------------------------------------------------------------------
		// State
		D3D11_VIEWPORT                    m_ViewPort;
		Mat4                              m_View;
		Mat4                              m_Projection;
		PBREnvironment                    m_Environment;
		RenderList                        m_GeometryPositionTexcoord;
		RenderList                        m_GeometryPositionNormalTexcoord;
		RenderList                        m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<SkinRenderItem>            m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<PointLight>                m_PointLights;
	};
}