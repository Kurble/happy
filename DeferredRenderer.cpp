#include "stdafx.h"
#include "DeferredRenderer.h"
#include "Sphere.h"
#include "Cube.h"
#include "Resources.h"

#include <algorithm>

//----------------------------------------------------------------------
// G-Buffer shaders
#include "CompiledShaders\VertexPositionTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTangentBinormalTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTangentBinormalTexcoordIndicesWeights.h"
#include "CompiledShaders\GeometryPS.h"
#include "CompiledShaders\GeometryAlphaStippledPS.h"
#include "CompiledShaders\DecalsPS.h"
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Rendering shaders
#include "CompiledShaders\ScreenQuadVS.h"
#include "CompiledShaders\SphereVS.h"
#include "CompiledShaders\SSAO.h"
#include "CompiledShaders\SSAOBlur.h"
#include "CompiledShaders\PointLighting.h"
#include "CompiledShaders\DeferredShadingPBR_ao_extreme.h"
#include "CompiledShaders\DeferredShadingPBR_ao_high.h"
#include "CompiledShaders\DeferredShadingPBR_ao_normal.h"
#include "CompiledShaders\DeferredShadingPBR_ao_low.h"
#include "CompiledShaders\DeferredShadingPBR_extreme.h"
#include "CompiledShaders\DeferredShadingPBR_high.h"
#include "CompiledShaders\DeferredShadingPBR_normal.h"
#include "CompiledShaders\DeferredShadingPBR_low.h"
//----------------------------------------------------------------------

namespace happy
{
	struct CBufferScene
	{
		bb::mat4 view;
		bb::mat4 projection;
		bb::mat4 viewInverse;
		bb::mat4 projectionInverse;
		float width;
		float height;
		unsigned int convolutionStages;
		unsigned int aoEnabled;
	};

	struct CBufferObject
	{
		bb::mat4 world;
		bb::mat4 worldInverse;
		float alpha;
	};

	struct CBufferSkin
	{
		float blendAnim[4];
		float blendFrame[4];
		unsigned int animationCount;
	};

	struct CBufferPointLight
	{
		bb::vec4 position;
		bb::vec4 color;
		float scale;
		float falloff;
	};

	struct CBufferSSAO
	{
		float occlusionRadius = 0.1f;
		int   samples         = 128;
		float invSamples      = 1.0f / 128.0f;

		bb::vec3 random[512];
	};

	DeferredRenderer::DeferredRenderer(const RenderingContext* pRenderContext, const RendererConfiguration config)
		: m_pRenderContext(pRenderContext)
		, m_Config(config)
	{
		m_TAA_Jitter.set(-0.33f, -0.33f);

		// Rasterizer state
		{
			D3D11_RASTERIZER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = 0;
			desc.SlopeScaledDepthBias = 0;
			desc.DepthBiasClamp = 0;
			desc.DepthClipEnable = true;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = false;
			desc.AntialiasedLineEnable = false;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateRasterizerState(&desc, &m_pRasterState));
		}

		// G-Buffer sampler state
		{
			D3D11_SAMPLER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.MinLOD = -FLT_MAX;
			desc.MaxLOD =  FLT_MAX;
			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateSamplerState(&desc, &m_pGSampler));
		}

		// Screen sampler state
		{
			D3D11_SAMPLER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.MinLOD = -FLT_MAX;
			desc.MaxLOD = FLT_MAX;
			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateSamplerState(&desc, &m_pScreenSampler));
		}

		// Per scene CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferScene) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBScene));
		}

		// Per object CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferObject) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBObject));
		}

		// Per skin CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferSkin) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBSkin));
		}

		// Per pointlight CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferPointLight) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBPointLighting));
		}

		// SSAO CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferSSAO) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBSSAO));

			CBufferSSAO ssaoCB;
			ssaoCB.occlusionRadius = m_Config.m_AOOcclusionRadius;
			ssaoCB.samples = m_Config.m_AOSamples;
			ssaoCB.invSamples = 1.0f / (float)m_Config.m_AOSamples;
			for (int i = 0; i < 512; ++i)
			{
				ssaoCB.random[i] = bb::vec3(-1.0f + (rand() % 2000) / 1000.0f, -1.0f + (rand() % 2000) / 1000.0f, (rand() % 1000) / 1000.0f);
				ssaoCB.random[i].normalize();
				float scale = (float)i / 512.0f;
				ssaoCB.random[i] *= bb::lerp(0.2f, 1.0f, scale * scale);
			}

			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(pRenderContext->getContext()->Map(m_pCBSSAO.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&ssaoCB, ((sizeof(CBufferSSAO) + 15) / 16) * 16);
			pRenderContext->getContext()->Unmap(m_pCBSSAO.Get(), 0);
		}

		// G-Buffer shaders
		CreateVertexShader<VertexPositionTexcoord>(pRenderContext->getDevice(), m_pVSPositionTexcoord, m_pILPositionTexcoord, g_shVertexPositionTexcoord);
		CreateVertexShader<VertexPositionNormalTexcoord>(pRenderContext->getDevice(), m_pVSPositionNormalTexcoord, m_pILPositionNormalTexcoord, g_shVertexPositionNormalTexcoord);
		CreateVertexShader<VertexPositionNormalTangentBinormalTexcoord>(pRenderContext->getDevice(), m_pVSPositionNormalTangentBinormalTexcoord, m_pILPositionNormalTangentBinormalTexcoord, g_shVertexPositionNormalTangentBinormalTexcoord);
		CreateVertexShader<VertexPositionNormalTangentBinormalTexcoordIndicesWeights>(pRenderContext->getDevice(), m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights, m_pILPositionNormalTangentBinormalTexcoordIndicesWeights, g_shVertexPositionNormalTangentBinormalTexcoordIndicesWeights);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSGeometry, g_shGeometryPS);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSGeometryAlphaStippled, g_shGeometryAlphaStippledPS);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSDecals, g_shDecalsPS);
		
		// Rendering shaders
		CreateVertexShader<VertexPositionTexcoord>(pRenderContext->getDevice(), m_pVSScreenQuad, m_pILScreenQuad, g_shScreenQuadVS);
		CreateVertexShader<VertexPositionTexcoord>(pRenderContext->getDevice(), m_pVSPointLighting, m_pILPointLighting, g_shSphereVS);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSSSAO, g_shSSAO);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSSSAOBlur, g_shSSAOBlur);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSPointLighting, g_shPointLighting);
		if (m_Config.m_PostEffectQuality >= Quality::Extreme) switch (m_Config.m_LightingQuality)
		{
			case Quality::Extreme: CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_ao_extreme); break;
			case Quality::High:    CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_ao_high);    break;
			case Quality::Normal:  CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_ao_normal);  break;
			case Quality::Low:     CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_ao_low);     break;
		}
		else switch (m_Config.m_LightingQuality)
		{
			case Quality::Extreme: CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_extreme);    break;
			case Quality::High:    CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_high);       break;
			case Quality::Normal:  CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_normal);     break;
			case Quality::Low:     CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shDeferredShadingPBR_low);        break;
		}

		// Noise texture
		{
			ComPtr<ID3D11Texture2D> pTexture;
			D3D11_TEXTURE2D_DESC desc;
			desc.Width = 4;
			desc.Height = 4;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			char data[4 * 4 * 4];
			for (int i = 0; i < sizeof(data); ++i)
			{
				data[i] = (char)rand();
			}

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = data;
			initData.SysMemPitch = static_cast<UINT>(4 * 4);
			initData.SysMemSlicePitch = static_cast<UINT>(sizeof(data));

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateTexture2D(&desc, &initData, &pTexture));
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateShaderResourceView(pTexture.Get(), nullptr, &m_pNoiseTexture));
		}

		// G-Buffer depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = true;
			desc.StencilReadMask = 0x00;
			desc.StencilWriteMask = 0xff;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.BackFace = desc.FrontFace;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateDepthStencilState(&desc, &m_pGBufferDepthStencilState));
		}

		// Decals depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = false;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = true;
			desc.StencilReadMask = 0xff;
			desc.StencilWriteMask = 0x00;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace = desc.FrontFace;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateDepthStencilState(&desc, &m_pDecalsDepthStencilState));
		}

		// Lighting depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = false;
			desc.StencilEnable = false;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateDepthStencilState(&desc, &m_pLightingDepthStencilState));
		}

		// Decals blending state
		{
			D3D11_BLEND_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask =
				D3D11_COLOR_WRITE_ENABLE_RED |
				D3D11_COLOR_WRITE_ENABLE_GREEN |
				D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.IndependentBlendEnable = false;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBlendState(&desc, &m_pDecalBlendState));
		}

		// Full screen quad
		{
			VertexPositionTexcoord quad[] =
			{
				quad[0] = { bb::vec4(-1, -1,  1,  1), bb::vec2(0, 1) },
				quad[1] = { bb::vec4( 1,  1,  1,  1), bb::vec2(1, 0) },
				quad[2] = { bb::vec4( 1, -1,  1,  1), bb::vec2(1, 1) },
				quad[3] = { bb::vec4(-1, -1,  1,  1), bb::vec2(0, 1) },
				quad[4] = { bb::vec4(-1,  1,  1,  1), bb::vec2(0, 0) },
				quad[5] = { bb::vec4( 1,  1,  1,  1), bb::vec2(1, 0) },
			};

			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(VertexPositionTexcoord) * 6;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)quad;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&desc, &data, &m_pScreenQuadBuffer));
		}

		// Sphere vtx buffer
		{
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(g_SphereVertices);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)g_SphereVertices;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&desc, &data, &m_pSphereVBuffer));
		}

		// Sphere idx buffer
		{
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(g_SphereIndices);
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)g_SphereIndices;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&desc, &data, &m_pSphereIBuffer));
		}

		// Cube vtx buffer
		{
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(g_CubeVertices);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)g_CubeVertices;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&desc, &data, &m_pCubeVBuffer));
		}

		// Cube idx buffer
		{
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(g_CubeIndices);
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)g_CubeIndices;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&desc, &data, &m_pCubeIBuffer));
		}
	}

	const RenderingContext* DeferredRenderer::getContext() const
	{
		return m_pRenderContext;
	}

	const RendererConfiguration& DeferredRenderer::getConfig() const
	{
		return m_Config;
	}

	void DeferredRenderer::setConfiguration(const RendererConfiguration &config)
	{
		m_Config = config;

		CBufferSSAO ssaoCB;
		ssaoCB.occlusionRadius = m_Config.m_AOOcclusionRadius;
		ssaoCB.samples = m_Config.m_AOSamples;
		ssaoCB.invSamples = 1.0f / (float)m_Config.m_AOSamples;
		for (int i = 0; i < 512; ++i)
		{
			ssaoCB.random[i] = bb::vec3(-1.0f + ((rand() % 2000) / 1000.0f), -1.0f + (rand() % 2000) / 1000.0f, (rand() % 1000) / 1000.0f);
			ssaoCB.random[i].normalize();
			float scale = (float)i / 512.0f;
			ssaoCB.random[i] *= bb::lerp(0.05f, 1.0f, scale * scale);
		}

		updateConstantBuffer(m_pRenderContext->getContext(), m_pCBSSAO.Get(), ssaoCB);
	}

	template <typename T>
	void DeferredRenderer::updateConstantBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, const T &value) const
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, (void*)&value, ((sizeof(T) + 15) / 16) * 16);
		context->Unmap(buffer, 0);
	}

	void DeferredRenderer::render(const RenderQueue *scene, RenderTarget *target) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.RSSetState(m_pRasterState.Get());
		context.RSSetViewports(1, &target->m_ViewPort);

		bb::mat4 jitteredProjection;
		jitteredProjection.identity();
		jitteredProjection.translate(bb::vec3(m_TAA_Jitter.x / (float)m_pRenderContext->getWidth(), m_TAA_Jitter.y / (float)m_pRenderContext->getHeight(), 0.0f));
		jitteredProjection.multiply(target->m_Projection);

		CBufferScene sceneCB;
		sceneCB.view = target->m_View;
		sceneCB.projection = jitteredProjection;
		sceneCB.viewInverse = target->m_View;
		sceneCB.viewInverse.inverse();
		sceneCB.projectionInverse = jitteredProjection;
		sceneCB.projectionInverse.inverse();
		sceneCB.width = (float)m_pRenderContext->getWidth();
		sceneCB.height = (float)m_pRenderContext->getHeight();
		sceneCB.convolutionStages = scene->m_Environment.getCubemapArrayLength();
		sceneCB.aoEnabled = m_Config.m_PostEffectQuality >= Quality::Normal ? 1 : 0;
		updateConstantBuffer<CBufferScene>(&context, m_pCBScene.Get(), sceneCB);

		renderGeometry(scene, target);

		renderDeferred(scene, target);
	}

	template<typename T>
	void DeferredRenderer::renderStaticMeshList(const vector<RenderQueue::MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		
		context.IASetInputLayout(layout);
		context.VSSetShader(shader, nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : renderList)
		{
			CBufferObject objectCB;
			objectCB.world = elem.m_Transform;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			UINT stride = sizeof(T);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Mesh.getVtxBuffer();

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Group);
			context.PSSetShaderResources(0, 3, elem.m_Mesh.getTextures());
			context.IASetIndexBuffer(elem.m_Mesh.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Mesh.getIndexCount(), 0, 0);
		}
	}

	void DeferredRenderer::renderGeometry(const RenderQueue *scene, RenderTarget *target) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();

		ID3D11RenderTargetView* rtvs[] = { target->m_GraphicsBuffer[0].rtv.Get(), target->m_GraphicsBuffer[1].rtv.Get(), target->m_GraphicsBuffer[2].rtv.Get() };
		context.OMSetRenderTargets(3, rtvs, target->m_pDepthBufferView.Get());
		context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context.ClearDepthStencilView(target->m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBObject.Get(),
			m_pCBSkin.Get(),
		};

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.PSSetShader(m_pPSGeometry.Get(), nullptr, 0);
		context.PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context.PSSetConstantBuffers(0, 3, constBuffers);

		//-------------------------------------------------------------
		// Render Static Meshes, opaque
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList<Vertex##X>(scene->m_Geometry##X, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//-------------------------------------------------------------
		// Render Skins, opaque
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights)
		{
			CBufferObject objectCB;
			objectCB.world = elem.m_World;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			CBufferSkin skinCB;
			skinCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 4; ++i) skinCB.blendAnim[i] = (&elem.m_BlendAnimation.x)[i];
			for (int i = 0; i < 4; ++i) skinCB.blendFrame[i] = (&elem.m_BlendFrame.x)[i];
			updateConstantBuffer(&context, m_pCBSkin.Get(), skinCB);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoordIndicesWeights);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Skin.getVtxBuffer();
			
			vector<ID3D11Buffer*> buffers = 
			{
				elem.m_Skin.getBindPoseBuffer()
			};

			for (unsigned i = 0; i < elem.m_AnimationCount * 2; ++i) buffers.push_back(elem.m_Frames[i]);

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Groups);
			context.VSSetConstantBuffers(3, (UINT)buffers.size(), &buffers[0]);
			context.PSSetShaderResources(0, 3, elem.m_Skin.getTextures());
			context.IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}

		context.PSSetShader(m_pPSGeometryAlphaStippled.Get(), nullptr, 0);
		context.PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context.PSSetConstantBuffers(0, 3, constBuffers);

		//-------------------------------------------------------------
		// Render Static Meshes, transparent
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList<Vertex##X>(scene->m_Geometry##X##Transparent, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//-------------------------------------------------------------
		// Render Skins, transparent
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent)
		{
			CBufferObject objectCB;
			objectCB.world = elem.m_World;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			CBufferSkin skinCB;
			skinCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 4; ++i) skinCB.blendAnim[i] = (&elem.m_BlendAnimation.x)[i];
			for (int i = 0; i < 4; ++i) skinCB.blendFrame[i] = (&elem.m_BlendFrame.x)[i];
			updateConstantBuffer(&context, m_pCBSkin.Get(), skinCB);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoordIndicesWeights);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Skin.getVtxBuffer();
			
			vector<ID3D11Buffer*> buffers =
			{
				elem.m_Skin.getBindPoseBuffer()
			};

			for (unsigned i = 0; i < elem.m_AnimationCount * 2; ++i) buffers.push_back(elem.m_Frames[i]);

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Groups);
			context.VSSetConstantBuffers(3, (UINT)buffers.size(), &buffers[0]);
			context.PSSetShaderResources(0, 3, elem.m_Skin.getTextures());
			context.IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}

		//-------------------------------------------------------------
		// Render Decals
		context.OMSetRenderTargets(3, rtvs, target->m_pDepthBufferViewReadOnly.Get());
		context.OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);
		context.IASetInputLayout(m_pILPositionTexcoord.Get());
		context.VSSetShader(m_pVSPositionTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		context.PSSetShader(m_pPSDecals.Get(), nullptr, 0);
		for (const auto &elem : scene->m_Decals)
		{
			CBufferObject objectCB;
			objectCB.world = elem.m_Transform;
			objectCB.worldInverse = elem.m_Transform;
			objectCB.worldInverse.inverse();
			objectCB.alpha = 1.0f;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			ID3D11Buffer* buffer = m_pCubeVBuffer.Get();

			ID3D11ShaderResourceView* textures[] =
			{
				elem.m_Texture.m_Handle.Get(),
				elem.m_NormalMap.m_Handle.Get(),
				nullptr,
				nullptr,
				nullptr,
				target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get(),
			};

			context.OMSetDepthStencilState(m_pDecalsDepthStencilState.Get(), elem.m_Filter);
			context.PSSetShaderResources(0, 6, textures);
			context.IASetIndexBuffer(m_pCubeIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, m_pCubeVBuffer.GetAddressOf(), &stride, &offset);
			context.DrawIndexed(36, 0, 0);
		}
	}

	void DeferredRenderer::renderDeferred(const RenderQueue *scene, RenderTarget *target) const
	{
		float clearColor[] = { 0, 0, 0, 0 };
		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		ID3D11ShaderResourceView* nullSRV = nullptr;

		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.OMSetDepthStencilState(m_pLightingDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);

		ID3D11SamplerState* samplers[] = { m_pScreenSampler.Get(), m_pGSampler.Get() };

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBPointLighting.Get()
		};

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(m_pILScreenQuad.Get());
		context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		context.PSSetSamplers(0, 2, samplers);
		context.PSSetConstantBuffers(0, 2, constBuffers);
		UINT stride = sizeof(VertexPositionTexcoord);
		UINT offset = 0;
		context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);
		
		//--------------------------------------------------------------------
		// Generate SSAO buffer
		if (m_Config.m_PostEffectQuality >= Quality::Normal)
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
				m_pCBSSAO.Get(),
			};

			context.RSSetViewports(1, &target->m_BlurViewPort);

			// Shader 1: SSAO
			context.OMSetRenderTargets(1, target->m_GraphicsBuffer[RenderTarget::GBuf_Occlusion0Idx].rtv.GetAddressOf(), nullptr);
			context.PSSetShader(m_pPSSSAO.Get(), nullptr, 0);
			context.PSSetConstantBuffers(0, 3, constBuffers);
			context.PSSetSamplers(0, 2, samplers);

			srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
			srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			srvs[6] = m_pNoiseTexture.Get();
			context.PSSetShaderResources(0, 8, srvs);

			context.Draw(6, 0);

			int aotarget = RenderTarget::GBuf_Occlusion1Idx;
			int aoview = RenderTarget::GBuf_Occlusion0Idx;

			// Shader 2+3: BLUR H+V
			for (int t = 0; t < 1; ++t)
			{
				context.PSSetShader(m_pPSSSAOBlur.Get(), nullptr, 0);
				context.PSSetConstantBuffers(0, 3, constBuffers);
				context.PSSetShaderResources(6, 1, &nullSRV);
				context.OMSetRenderTargets(1, target->m_GraphicsBuffer[aotarget].rtv.GetAddressOf(), nullptr);
				srvs[6] = target->m_GraphicsBuffer[aoview].srv.Get();
				context.PSSetShaderResources(0, 8, srvs);

				context.Draw(6, 0);

				std::swap(aotarget, aoview);
			}

			context.RSSetViewports(1, &target->m_ViewPort);
		}

		//--------------------------------------------------------------------
		// SRVs State
		srvs[0] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics0Idx].srv.Get();
		srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
		srvs[2] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics2Idx].srv.Get();
		srvs[3] = nullptr;
		srvs[4] = target->m_GraphicsBuffer[RenderTarget::GBuf_Occlusion1Idx].srv.Get();
		srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
		srvs[6] = scene->m_Environment.getLightingSRV();
		srvs[7] = scene->m_Environment.getEnvironmentSRV();

		//--------------------------------------------------------------------
		// Render lighting
		{
			ID3D11RenderTargetView* rtvs[] = { scene->m_PostProcessItems.size() > 0 ? target->m_PostBuffer[0].rtv.Get() : m_pRenderContext->getBackBuffer() };
			context.OMSetRenderTargets(1, rtvs, nullptr);
			context.PSSetShaderResources(0, 8, srvs);

			// Render environmental lighting
			context.PSSetShader(m_pPSGlobalLighting.Get(), nullptr, 0);
			context.Draw(6, 0);

			// Render point lights
			// no overdraw pls, put the lights in the pbr shader

			/*context.VSSetShader(m_pVSPointLighting.Get(), nullptr, 0);
			context.IASetInputLayout(m_pILPointLighting.Get());
			context.IASetVertexBuffers(0, 1, m_pSphereVBuffer.GetAddressOf(), &stride, &offset);
			context.IASetIndexBuffer(m_pSphereIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context.PSSetShader(m_pPSPointLighting.Get(), nullptr, 0);

			for (auto &light : m_PointLights)
			{
				CBufferPointLight cbuf;
				cbuf.position = bb::vec4(light.m_Position.x, light.m_Position.y, light.m_Position.z, 0.0f);
				cbuf.color    = bb::vec4(light.m_Color.x, light.m_Color.y, light.m_Color.z, 0.0f);
				cbuf.scale    = light.m_Radius;
				cbuf.falloff  = light.m_FaloffExponent;

				D3D11_MAPPED_SUBRESOURCE msr;
				THROW_ON_FAIL(context.Map(m_pCBPointLighting.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
				memcpy(msr.pData, (void*)&cbuf, ((sizeof(CBufferPointLight) + 15) / 16) * 16);
				context.Unmap(m_pCBPointLighting.Get(), 0);

				context.DrawIndexed(sizeof(g_SphereIndices) / sizeof(uint16_t), 0, 0);
			}*/
		}

		//--------------------------------------------------------------------
		// Post processing
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
			};

			context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context.IASetInputLayout(m_pILScreenQuad.Get());
			context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
			context.VSSetConstantBuffers(0, 2, constBuffers);
			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);

			int pptarget = 1;
			int ppview = 0;

			for (auto process = scene->m_PostProcessItems.begin(); process != scene->m_PostProcessItems.end(); ++process)
			{
				ID3D11RenderTargetView* pp_rtvs[] = { target->m_PostBuffer[pptarget].rtv.Get() };
				if (process == scene->m_PostProcessItems.end() - 1)
				{
					pp_rtvs[0] = m_pRenderContext->getBackBuffer();
				}

				ID3D11ShaderResourceView* pp_srvs[10] = { 0 };
				if (process->m_SceneInputSlot < 10) 
					pp_srvs[process->m_SceneInputSlot] = target->m_PostBuffer[ppview].srv.Get();
				if (process->m_DepthInputSlot < 10) 
					pp_srvs[process->m_DepthInputSlot] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
				for (auto &slot : process->m_InputSlots)
					pp_srvs[slot.first] = (ID3D11ShaderResourceView*)slot.second;

				if (process->m_ConstBuffer)
				{
					D3D11_MAPPED_SUBRESOURCE msr;
					THROW_ON_FAIL(context.Map(process->m_ConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
					memcpy(msr.pData, (void*)process->m_ConstBufferData->data(), process->m_ConstBufferData->size());
					context.Unmap(process->m_ConstBuffer.Get(), 0);

					constBuffers[1] = process->m_ConstBuffer.Get();
				}
				else
				{
					constBuffers[1] = nullptr;
				}

				context.OMSetRenderTargets(1, pp_rtvs, nullptr);
				context.PSSetShader(process->m_Handle.Get(), nullptr, 0);
				context.PSSetShaderResources(0, 10, pp_srvs);
				context.PSSetSamplers(0, 2, samplers);
				context.PSSetConstantBuffers(0, 2, constBuffers);
				
				context.Draw(6, 0);

				std::swap(pptarget, ppview);

				context.PSSetShaderResources(process->m_SceneInputSlot, 1, &nullSRV);
			}
		}

		// Reset SRVs since we need them as output next frame
		for (int i = 0; i < 8; ++i) srvs[i] = nullptr;
		context.PSSetShaderResources(0, 8, srvs);
	}
}