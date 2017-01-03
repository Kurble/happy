#include "stdafx.h"
#include "DeferredRenderer.h"
#include "DeferredRendererConstBuffers.h"
#include "Sphere.h"
#include "Cube.h"
#include "Resources.h"

#include "bb_lib\halton.h"

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
#include "CompiledShaders\TAA.h"
#include "CompiledShaders\PointLighting.h"
#include "CompiledShaders\DeferredShadingPBR_ao_extreme.h"
#include "CompiledShaders\DeferredShadingPBR_ao_high.h"
#include "CompiledShaders\DeferredShadingPBR_ao_normal.h"
#include "CompiledShaders\DeferredShadingPBR_ao_low.h"
#include "CompiledShaders\DeferredShadingPBR_extreme.h"
#include "CompiledShaders\DeferredShadingPBR_high.h"
#include "CompiledShaders\DeferredShadingPBR_normal.h"
#include "CompiledShaders\DeferredShadingPBR_low.h"
#include "CompiledShaders\ColorGrading.h"
//----------------------------------------------------------------------

namespace happy
{
	void DeferredRenderer::createStates(const RenderingContext *pRenderContext)
	{
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
			desc.MaxLOD = FLT_MAX;
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
	}

	void DeferredRenderer::createGeometries(const RenderingContext *pRenderContext)
	{
		// Full screen quad
		{
			VertexPositionTexcoord quad[] =
			{
				quad[0] = { bb::vec4(-1, -1,  1,  1), bb::vec2(0, 1) },
				quad[1] = { bb::vec4(1,  1,  1,  1), bb::vec2(1, 0) },
				quad[2] = { bb::vec4(1, -1,  1,  1), bb::vec2(1, 1) },
				quad[3] = { bb::vec4(-1, -1,  1,  1), bb::vec2(0, 1) },
				quad[4] = { bb::vec4(-1,  1,  1,  1), bb::vec2(0, 0) },
				quad[5] = { bb::vec4(1,  1,  1,  1), bb::vec2(1, 0) },
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

	void DeferredRenderer::createBuffers(const RenderingContext *pRenderContext)
	{
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

		// TAA CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferTAA) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBTAA));
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
				float xd = ((i) % 2) ? 1.0f : -1.0f;
				float yd = ((i / 2) % 2) ? 1.0f : -1.0f;
				ssaoCB.random[i] = bb::vec3(xd * (rand() % 1000) / 1000.0f, 
					                        yd * (rand() % 1000) / 1000.0f, 
					                             (rand() % 1000) / 1000.0f);
				ssaoCB.random[i].normalize();
				float scale = (float)(i%m_Config.m_AOSamples) / (float)m_Config.m_AOSamples;
				ssaoCB.random[i] *= bb::lerp(0.1f, 1.0f, scale * scale);
			}

			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(pRenderContext->getContext()->Map(m_pCBSSAO.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&ssaoCB, ((sizeof(CBufferSSAO) + 15) / 16) * 16);
			pRenderContext->getContext()->Unmap(m_pCBSSAO.Get(), 0);
		}

		// Noise texture
		{
			D3D11_TEXTURE2D_DESC desc;
			desc.Width = 4;
			desc.Height = 4;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			unsigned char data[4 * 4 * 2];

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = data;
			initData.SysMemPitch = static_cast<UINT>(4 * 4);
			initData.SysMemSlicePitch = static_cast<UINT>(sizeof(data));

			for (unsigned tex = 0; tex < RenderTarget::MultiSamples; ++tex)
			{
				for (int i = 0; i < sizeof(data); i+=2)
				{
					int j = i + tex;
					int k = j + 1;
					int ref = ((j / 4) % 2) ? 0 : 1;
					float xd = (j % 2) == ref ? 1.0f : -1.0f;
					float yd = ((k / 2) % 2) == ref ? 1.0f : -1.0f;

					bb::vec2 direction = bb::vec2(
						(rand() / (float)RAND_MAX) * xd,
						(rand() / (float)RAND_MAX) * yd).normalized();

					direction.x = 0.5f * direction.x + 0.5f;
					direction.y = 0.5f * direction.y + 0.5f;

					data[i + 0] = (unsigned char)(255.0f * direction.x);
					data[i + 1] = (unsigned char)(255.0f * direction.y);
				}

				ComPtr<ID3D11Texture2D> pTexture;
				THROW_ON_FAIL(pRenderContext->getDevice()->CreateTexture2D(&desc, &initData, &pTexture));
				THROW_ON_FAIL(pRenderContext->getDevice()->CreateShaderResourceView(pTexture.Get(), nullptr, &m_pNoiseTexture[tex]));
			}
		}
	}

	void DeferredRenderer::createShaders(const RenderingContext *pRenderContext)
	{
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
		CreatePixelShader(pRenderContext->getDevice(), m_pPSTAA, g_shTAA);
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

		CreatePixelShader(pRenderContext->getDevice(), m_ColorGrading.m_Handle, g_shColorGrading);
	}
}