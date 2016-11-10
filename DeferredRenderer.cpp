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
#include "CompiledShaders\DeferredDirectionalOcclusion.h"
#include "CompiledShaders\EdgePreserveBlur.h"
#include "CompiledShaders\EnvironmentalLighting.h"
#include "CompiledShaders\PointLighting.h"
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

	struct CBufferEffects
	{
		float occlusionRadius = 0.1f;
		float occlusionMaxDistance = 10.2f;
		bb::vec2  blurDir;

		int   samples = 128;
	};

	struct CBufferRandom
	{
		bb::vec2 random_points[512];
	};

	DeferredRenderer::DeferredRenderer(const RenderingContext* pRenderContext)
		: m_pRenderContext(pRenderContext)
	{
		m_View.identity();
		m_Projection.identity();

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

		// Random CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferRandom) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBRandom));

			CBufferRandom randomCB;

			float xs[512], ys[512];
			for (unsigned i = 0; i < 512; ++i)
			{
				int slotI = -4 + (i % 8);
				float slotF = slotI / 4.0f;
				xs[i] = slotF + (rand() / (float)(RAND_MAX * 4));
				ys[i] = slotF + (rand() / (float)(RAND_MAX * 4));
			}
			for (unsigned i = 0; i < 512; i += 32)
			{
				std::random_shuffle(&xs[i], &xs[i + 32]);
				std::random_shuffle(&ys[i], &ys[i + 32]);
			}
			for (unsigned i = 0; i < 512; ++i)
			{
				randomCB.random_points[i] = { xs[i], ys[i] };
			}

			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(pRenderContext->getContext()->Map(m_pCBRandom.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&randomCB, ((sizeof(CBufferRandom) + 15) / 16) * 16);
			pRenderContext->getContext()->Unmap(m_pCBRandom.Get(), 0);
		}

		// Effects CB
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.ByteWidth = (UINT)((sizeof(CBufferEffects) + 15) / 16) * 16;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBEffects[0]));
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&bufferDesc, NULL, &m_pCBEffects[1]));
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
		CreatePixelShader(pRenderContext->getDevice(), m_pPSDSSDO, g_shDeferredDirectionalOcclusion);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSBlur, g_shEdgePreserveBlur);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSGlobalLighting, g_shEnvironmentalLighting);
		CreatePixelShader(pRenderContext->getDevice(), m_pPSPointLighting, g_shPointLighting);

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

			char data[16 * 4];
			for (int i = 0; i < sizeof(data); ++i)
			{
				data[i] = (char)rand();
			}

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = data;
			initData.SysMemPitch = static_cast<UINT>(16);
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

		// Lighting blending state
		{
			D3D11_BLEND_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = 
				D3D11_COLOR_WRITE_ENABLE_RED |
				D3D11_COLOR_WRITE_ENABLE_GREEN | 
				D3D11_COLOR_WRITE_ENABLE_BLUE;
			desc.IndependentBlendEnable = false;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBlendState(&desc, &m_pRenderBlendState));

			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBlendState(&desc, &m_pDefaultBlendState));
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

	const bb::mat4 DeferredRenderer::getViewProj() const
	{
		bb::mat4 viewproj;
		viewproj.identity();
		viewproj.multiply(m_Projection);
		viewproj.multiply(m_View);
		return viewproj;
	}

	void DeferredRenderer::resize(unsigned int width, unsigned int height)
	{
		ID3D11Device& device = *m_pRenderContext->getDevice();

		m_ViewPort.Width = (float)width;
		m_ViewPort.Height = (float)height;
		m_ViewPort.MinDepth = 0.0f;
		m_ViewPort.MaxDepth = 1.0f;
		m_ViewPort.TopLeftX = 0;
		m_ViewPort.TopLeftY = 0;

		m_BlurViewPort = m_ViewPort;
		if (!m_Config.m_AOHiRes)
		{
			m_BlurViewPort.Width = (float)width / 2.0f;
			m_BlurViewPort.Height = (float)height / 2.0f;
		}

		// Create G-Buffer
		vector<unsigned char> texture(width * height * 4, 0);
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = texture.data();
		data.SysMemPitch = width * 4;

		for (unsigned int i = 0; i < 4; ++i)
		{
			if ((i == 2 || i == 3) && !m_Config.m_AOHiRes)
			{
				texDesc.Width = width / 2;
				texDesc.Height = height / 2;
			}
			else
			{
				texDesc.Width = width;
				texDesc.Height = height;
			}

			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &m_pGBuffer[i]));
			THROW_ON_FAIL(device.CreateRenderTargetView(m_pGBuffer[i].Get(), nullptr, m_pGBufferTarget[i].GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(m_pGBuffer[i].Get(), nullptr, m_pGBufferView[i].GetAddressOf()));
		}

		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &m_pGBuffer[4]));

		// depth-stencil buffer object
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = 0;
			THROW_ON_FAIL(device.CreateDepthStencilView(m_pGBuffer[4].Get(), &dsvDesc, m_pDepthBufferView.GetAddressOf()));
		}

		// depth-stencil buffer object (read only)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
			THROW_ON_FAIL(device.CreateDepthStencilView(m_pGBuffer[4].Get(), &dsvDesc, m_pDepthBufferViewReadOnly.GetAddressOf()));
		}

		// depth buffer view
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1;
			THROW_ON_FAIL(device.CreateShaderResourceView(m_pGBuffer[4].Get(), &srvDesc, m_pGBufferView[4].GetAddressOf()));
		}

		for (unsigned int i = 0; i < 2; ++i)
		{
			ComPtr<ID3D11Texture2D> texHandle;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &texHandle));
			THROW_ON_FAIL(device.CreateRenderTargetView(texHandle.Get(), nullptr, m_pPostProcessRT[i].GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(texHandle.Get(), nullptr, m_pPostProcessView[i].GetAddressOf()));
		}
	}

	void DeferredRenderer::clear()
	{
		m_GeometryPositionTexcoord.clear();
		m_GeometryPositionNormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.clear();
		m_GeometryPositionTexcoordTransparent.clear();
		m_GeometryPositionNormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.clear();
		m_Decals.clear();
		m_PointLights.clear();
		m_PostProcessItems.clear();
	}

	void DeferredRenderer::pushSkinRenderItem(const SkinRenderItem &skin)
	{
		if (skin.m_Alpha < 1.0f)
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.push_back(skin);
		else
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.push_back(skin);
	}

	void DeferredRenderer::pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group)
	{
		switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		}
	}

	void DeferredRenderer::pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group)
	{
		if (alpha >= 1.0f) pushRenderMesh(mesh, transform, group);
		else switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		}
	}

	void DeferredRenderer::pushLight(const bb::vec3 &position, const bb::vec3 &color, float radius, float falloff)
	{
		m_PointLights.emplace_back(position, color, radius, falloff);
	}

	void DeferredRenderer::pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter)
	{
		TextureHandle emptyHandle;
		m_Decals.emplace_back(texture, emptyHandle, transform, filter);
	}

	void DeferredRenderer::pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Decals.emplace_back(texture, normalMap, transform, filter);
	}

	void DeferredRenderer::pushPostProcessItem(const PostProcessItem &proc)
	{
		m_PostProcessItems.push_back(proc);
	}

	void DeferredRenderer::setEnvironment(const PBREnvironment &environment)
	{
		m_Environment = environment;
	}

	void DeferredRenderer::setCamera(const bb::mat4 &view, const bb::mat4 &projection)
	{
		m_View = view;
		m_Projection = projection;
	}

	void DeferredRenderer::setConfiguration(const RendererConfiguration &config)
	{
		m_Config = config;
	}

	template <typename T>
	void DeferredRenderer::updateConstantBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, const T &value) const
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, (void*)&value, ((sizeof(T) + 15) / 16) * 16);
		context->Unmap(buffer, 0);
	}

	void DeferredRenderer::render() const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.RSSetState(m_pRasterState.Get());
		context.RSSetViewports(1, &m_ViewPort);

		CBufferScene sceneCB;
		sceneCB.view = m_View;
		sceneCB.projection = m_Projection;
		sceneCB.viewInverse = m_View;
		sceneCB.viewInverse.inverse();
		sceneCB.projectionInverse = m_Projection;
		sceneCB.projectionInverse.inverse();
		sceneCB.width = (float)m_pRenderContext->getWidth();
		sceneCB.height = (float)m_pRenderContext->getHeight();
		sceneCB.convolutionStages = m_Environment.getCubemapArrayLength();
		sceneCB.aoEnabled = m_Config.m_AOEnabled ? 1 : 0;
		updateConstantBuffer<CBufferScene>(&context, m_pCBScene.Get(), sceneCB);
		
		CBufferEffects effectsCB;
		effectsCB.occlusionRadius = m_Config.m_AOOcclusionRadius;
		effectsCB.occlusionMaxDistance = m_Config.m_AOOcclusionMaxDistance;
		effectsCB.samples = m_Config.m_AOSamples;
		for (int i = 0; i < 2; ++i)
		{
			effectsCB.blurDir = i ? bb::vec2(0, 1) : bb::vec2(1, 0);
			updateConstantBuffer(&context, m_pCBEffects[i].Get(), effectsCB);
		}

		renderGeometryToGBuffer();
		renderGBufferToBackBuffer();
	}

	template<typename T>
	void DeferredRenderer::renderStaticMeshList(const vector<MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const
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

	void DeferredRenderer::renderGeometryToGBuffer() const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();

		ID3D11RenderTargetView* rtvs[] = { m_pGBufferTarget[0].Get(), m_pGBufferTarget[1].Get() };
		context.OMSetRenderTargets(2, rtvs, m_pDepthBufferView.Get());
		context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);

		float cc[4] = { 0, 0, 0, 0 };
		context.ClearRenderTargetView(m_pGBufferTarget[0].Get(), cc);
		context.ClearDepthStencilView(m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);

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
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList<Vertex##X>(m_Geometry##X, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//-------------------------------------------------------------
		// Render Skins, opaque
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights)
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
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList<Vertex##X>(m_Geometry##X##Transparent, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//-------------------------------------------------------------
		// Render Skins, transparent
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent)
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
		context.OMSetRenderTargets(2, rtvs, m_pDepthBufferViewReadOnly.Get());
		context.OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);
		context.IASetInputLayout(m_pILPositionTexcoord.Get());
		context.VSSetShader(m_pVSPositionTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		context.PSSetShader(m_pPSDecals.Get(), nullptr, 0);
		for (const auto &elem : m_Decals)
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
				m_pGBufferView[4].Get(),
			};

			context.OMSetDepthStencilState(m_pDecalsDepthStencilState.Get(), elem.m_Filter);
			context.PSSetShaderResources(0, 3, textures);
			context.IASetIndexBuffer(m_pCubeIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, m_pCubeVBuffer.GetAddressOf(), &stride, &offset);
			context.DrawIndexed(36, 0, 0);
		}
	}

	void DeferredRenderer::renderGBufferToBackBuffer() const
	{
		float clearColor[] = { 0, 0, 0, 0 };
		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		ID3D11ShaderResourceView* nullSRV = nullptr;

		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.OMSetDepthStencilState(m_pLightingDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context.ClearRenderTargetView(m_pRenderContext->getBackBuffer(), clearColor);
		context.ClearRenderTargetView(m_pPostProcessRT[0].Get(), clearColor);
		context.ClearRenderTargetView(m_pPostProcessRT[1].Get(), clearColor);

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
		// Generate DSSDO buffer
		if (m_Config.m_AOEnabled)
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBEffects[0].Get(),
				m_pCBRandom.Get(),
			};

			context.RSSetViewports(1, &m_BlurViewPort);

			// Shader 1: DSSDO
			context.OMSetRenderTargets(1, m_pGBufferTarget[2].GetAddressOf(), nullptr);
			context.PSSetShader(m_pPSDSSDO.Get(), nullptr, 0);
			context.PSSetConstantBuffers(2, 2, constBuffers);

			srvs[0] = m_pGBufferView[0].Get();
			srvs[1] = m_pGBufferView[1].Get();
			srvs[3] = m_pGBufferView[4].Get();
			srvs[4] = m_pNoiseTexture.Get();
			context.PSSetShaderResources(0, 6, srvs);

			context.Draw(6, 0);

			int target = 3;
			int view = 2;

			// Shader 2+3: BLUR H+V
			context.PSSetShader(m_pPSBlur.Get(), nullptr, 0);
			for (int t = 0; t < 2; ++t)
			{
				context.PSSetShaderResources(2, 1, &nullSRV);
				context.OMSetRenderTargets(1, m_pGBufferTarget[target].GetAddressOf(), nullptr);

				srvs[2] = m_pGBufferView[view].Get();
				context.PSSetShaderResources(0, 6, srvs);

				constBuffers[0] = m_pCBEffects[t].Get();
				context.PSSetConstantBuffers(2, 2, constBuffers);

				context.Draw(6, 0);

				std::swap(target, view);
			}

			context.RSSetViewports(1, &m_ViewPort);
		}

		//--------------------------------------------------------------------
		// Render lighting
		{
			ID3D11RenderTargetView* rtvs[] = { m_PostProcessItems.size() > 0 ? m_pPostProcessRT[0].Get() : m_pRenderContext->getBackBuffer() };
			context.OMSetRenderTargets(1, rtvs, nullptr);
			context.OMSetBlendState(m_pRenderBlendState.Get(), nullptr, 0xffffffff);
			srvs[2] = m_pGBufferView[2].Get();
			srvs[4] = m_Environment.getLightingSRV();
			srvs[5] = m_Environment.getEnvironmentSRV();
			context.PSSetShaderResources(0, 6, srvs);

			// Render environmental lighting
			context.PSSetShader(m_pPSGlobalLighting.Get(), nullptr, 0);
			context.Draw(6, 0);

			// Render point lights
			context.VSSetShader(m_pVSPointLighting.Get(), nullptr, 0);
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
			}
		}

		//--------------------------------------------------------------------
		// Post processing
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
			};

			context.OMSetBlendState(m_pDefaultBlendState.Get(), nullptr, 0xffffffff);
			context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context.IASetInputLayout(m_pILScreenQuad.Get());
			context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
			context.VSSetConstantBuffers(0, 2, constBuffers);
			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);

			int target = 1;
			int view = 0;

			for (auto process = m_PostProcessItems.begin(); process != m_PostProcessItems.end(); ++process)
			{
				ID3D11RenderTargetView* pp_rtvs[] = { m_pPostProcessRT[target].Get() };
				if (process == m_PostProcessItems.end() - 1)
				{
					pp_rtvs[0] = m_pRenderContext->getBackBuffer();
				}

				ID3D11ShaderResourceView* pp_srvs[10] = { 0 };
				if (process->m_SceneInputSlot < 10) 
					pp_srvs[process->m_SceneInputSlot] = m_pPostProcessView[view].Get();
				if (process->m_DepthInputSlot < 10) 
					pp_srvs[process->m_DepthInputSlot] = m_pGBufferView[4].Get();
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
				context.PSSetShaderResources(0, 6, pp_srvs);
				context.PSSetSamplers(0, 2, samplers);
				context.PSSetConstantBuffers(0, 2, constBuffers);
				
				context.Draw(6, 0);

				std::swap(target, view);

				context.PSSetShaderResources(process->m_SceneInputSlot, 1, &nullSRV);
			}
		}

		// Reset SRVs since we need them as output next frame
		for (int i = 0; i < 4; ++i) srvs[i] = nullptr;
		context.PSSetShaderResources(0, 6, srvs);
	}
}