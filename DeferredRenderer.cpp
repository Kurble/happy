#include "stdafx.h"
#include "DeferredRenderer.h"
#include "Sphere.h"

#include <algorithm>

//----------------------------------------------------------------------
// G-Buffer shaders
#include "CompiledShaders\VertexPositionTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTangentBinormalTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTangentBinormalTexcoordIndicesWeights.h"
#include "CompiledShaders\GeometryPS.h"
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
		CreateVertexShader<VertexPositionTexcoord>(m_pVSPositionTexcoord, m_pILPositionTexcoord, g_shVertexPositionTexcoord);
		CreateVertexShader<VertexPositionNormalTexcoord>(m_pVSPositionNormalTexcoord, m_pILPositionNormalTexcoord, g_shVertexPositionNormalTexcoord);
		CreateVertexShader<VertexPositionNormalTangentBinormalTexcoord>(m_pVSPositionNormalTangentBinormalTexcoord, m_pILPositionNormalTangentBinormalTexcoord, g_shVertexPositionNormalTangentBinormalTexcoord);
		CreateVertexShader<VertexPositionNormalTangentBinormalTexcoordIndicesWeights>(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights, m_pILPositionNormalTangentBinormalTexcoordIndicesWeights, g_shVertexPositionNormalTangentBinormalTexcoordIndicesWeights);
		CreatePixelShader(m_pPSGeometry, g_shGeometryPS);
		
		// Rendering shaders
		CreateVertexShader<VertexPositionTexcoord>(m_pVSScreenQuad, m_pILScreenQuad, g_shScreenQuadVS);
		CreateVertexShader<VertexPositionTexcoord>(m_pVSPointLighting, m_pILPointLighting, g_shSphereVS);
		CreatePixelShader(m_pPSDSSDO, g_shDeferredDirectionalOcclusion);
		CreatePixelShader(m_pPSBlur, g_shEdgePreserveBlur);
		CreatePixelShader(m_pPSGlobalLighting, g_shEnvironmentalLighting);
		CreatePixelShader(m_pPSPointLighting, g_shPointLighting);

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

		// Rendering depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = false;
			desc.StencilEnable = false;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateDepthStencilState(&desc, &m_pRenderDepthState));
		}

		// Rendering blending state
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
		}

		// Full screen quad
		{
			VertexPositionTexcoord quad[] =
			{
				quad[0] = { Vec4(-1, -1,  1,  1), Vec2(0, 1) },
				quad[1] = { Vec4( 1,  1,  1,  1), Vec2(1, 0) },
				quad[2] = { Vec4( 1, -1,  1,  1), Vec2(1, 1) },
				quad[3] = { Vec4(-1, -1,  1,  1), Vec2(0, 1) },
				quad[4] = { Vec4(-1,  1,  1,  1), Vec2(0, 0) },
				quad[5] = { Vec4( 1,  1,  1,  1), Vec2(1, 0) },
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
	}

	const RenderingContext* DeferredRenderer::getContext() const
	{
		return m_pRenderContext;
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
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &m_pGBuffer[4]));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = 0;
		THROW_ON_FAIL(device.CreateDepthStencilView(m_pGBuffer[4].Get(), &dsvDesc, m_pDepthBufferView.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		THROW_ON_FAIL(device.CreateShaderResourceView(m_pGBuffer[4].Get(), &srvDesc, m_pGBufferView[4].GetAddressOf()));
	}

	void DeferredRenderer::clear()
	{
		m_GeometryPositionTexcoord.clear();
		m_GeometryPositionNormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.clear();

		m_PointLights.clear();
	}

	void DeferredRenderer::pushSkinRenderItem(const SkinRenderItem &skin)
	{
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.push_back(skin);
	}

	void DeferredRenderer::pushRenderMesh(const RenderMesh &mesh, const Mat4 &transform)
	{
		switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoord.emplace_back(mesh, transform);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoord.emplace_back(mesh, transform);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoord.emplace_back(mesh, transform);
			break;
		}
	}

	void DeferredRenderer::pushLight(const Vec3 &position, const Vec3 &color, float radius, float falloff)
	{
		PointLight pl = { position, color, radius, falloff };
		m_PointLights.emplace_back(pl);
	}

	void DeferredRenderer::setEnvironment(const PBREnvironment &environment)
	{
		m_Environment = environment;
	}

	void DeferredRenderer::setCamera(const Mat4 &view, const Mat4 &projection)
	{
		m_View = view;
		m_Projection = projection;
	}

	void DeferredRenderer::setConfiguration(const RendererConfiguration &config)
	{
		m_Config = config;
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

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(context.Map(m_pCBScene.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, (void*)&sceneCB, ((sizeof(CBufferScene) + 15) / 16) * 16);
		context.Unmap(m_pCBScene.Get(), 0);

		CBufferEffects effectsCB;
		effectsCB.occlusionRadius = m_Config.m_AOOcclusionRadius;
		effectsCB.occlusionMaxDistance = m_Config.m_AOOcclusionMaxDistance;
		effectsCB.samples = m_Config.m_AOSamples;

		for (int i = 0; i < 2; ++i)
		{
			effectsCB.blurDir = i ? Vec2(0, 1) : Vec2(1, 0);

			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(m_pCBEffects[i].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&effectsCB, ((sizeof(CBufferEffects) + 15) / 16) * 16);
			context.Unmap(m_pCBEffects[i].Get(), 0);
		}

		renderGeometryToGBuffer();
		renderGBufferToBackBuffer();
	}

	void DeferredRenderer::renderGeometryToGBuffer() const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();

		ID3D11RenderTargetView* rtvs[] = { m_pGBufferTarget[0].Get(), m_pGBufferTarget[1].Get() };
		context.OMSetRenderTargets(2, rtvs, m_pDepthBufferView.Get());
		context.OMSetDepthStencilState(nullptr, 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);

		float cc[4] = { 0, 0, 0, 0 };
		context.ClearRenderTargetView(m_pGBufferTarget[0].Get(), cc);
		context.ClearDepthStencilView(m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.PSSetShader(m_pPSGeometry.Get(), nullptr, 0);
		context.PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBObject.Get()
		};
		CBufferObject objectCB;

		//-------------------------------------------------------------
		// Render VertexPositionTexcoord elements
		context.IASetInputLayout(m_pILPositionTexcoord.Get());
		context.VSSetShader(m_pVSPositionTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		for (const auto &elem : m_GeometryPositionTexcoord)
		{
			objectCB.world = elem.second;
			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(m_pCBObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferObject) + 15) / 16) * 16);
			context.Unmap(m_pCBObject.Get(), 0);

			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.first.getVtxBuffer();

			ID3D11ShaderResourceView* textures[] = 
			{
				elem.first.getAlbedoRoughnessMap(),
				elem.first.getNormalMetallicMap()
			};

			context.PSSetShaderResources(0, 2, textures);
			context.IASetIndexBuffer(elem.first.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.first.getIndexCount(), 0, 0);
		}

		//-------------------------------------------------------------
		// Render VertexPositionNormalTexcoord elements
		context.IASetInputLayout(m_pILPositionNormalTexcoord.Get());
		context.VSSetShader(m_pVSPositionNormalTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		for (const auto &elem : m_GeometryPositionNormalTexcoord)
		{
			objectCB.world = elem.second;
			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(m_pCBObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferObject) + 15) / 16) * 16);
			context.Unmap(m_pCBObject.Get(), 0);

			UINT stride = sizeof(VertexPositionNormalTexcoord);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.first.getVtxBuffer();

			ID3D11ShaderResourceView* textures[] =
			{
				elem.first.getAlbedoRoughnessMap(),
				elem.first.getNormalMetallicMap()
			};

			context.PSSetShaderResources(0, 2, textures);
			context.IASetIndexBuffer(elem.first.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.first.getIndexCount(), 0, 0);
		}

		//-------------------------------------------------------------
		// Render VertexPositionNormalTangentBinormalTexcoord elements
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoord.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		for (const auto &elem : m_GeometryPositionNormalTangentBinormalTexcoord)
		{
			objectCB.world = elem.second;
			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(m_pCBObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferObject) + 15) / 16) * 16);
			context.Unmap(m_pCBObject.Get(), 0);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoord);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.first.getVtxBuffer();

			ID3D11ShaderResourceView* textures[] =
			{
				elem.first.getAlbedoRoughnessMap(),
				elem.first.getNormalMetallicMap()
			};

			context.PSSetShaderResources(0, 2, textures);
			context.IASetIndexBuffer(elem.first.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.first.getIndexCount(), 0, 0);
		}

		//-------------------------------------------------------------
		// Render VertexPositionNormalTangentBinormalTexcoordIndicesWeights elements
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		for (const auto &elem : m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights)
		{
			objectCB.world = elem.m_World;
			objectCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 4; ++i) objectCB.blendAnim[i] = (&elem.m_BlendAnimation.x)[i];
			for (int i = 0; i < 4; ++i) objectCB.blendFrame[i] = (&elem.m_BlendFrame.x)[i];
			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(m_pCBObject.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferObject) + 15) / 16) * 16);
			context.Unmap(m_pCBObject.Get(), 0);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoordIndicesWeights);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Skin.getVtxBuffer();

			ID3D11ShaderResourceView* textures[] =
			{
				elem.m_Skin.getAlbedoRoughnessMap(),
				elem.m_Skin.getNormalMetallicMap()
			};

			vector<ID3D11Buffer*> buffers = 
			{
				elem.m_Skin.getBindPoseBuffer()
			};

			for (unsigned i = 0; i < elem.m_AnimationCount * 2; ++i) buffers.push_back(elem.m_Frames[i]);

			context.VSSetConstantBuffers(2, (UINT)buffers.size(), &buffers[0]);
			context.PSSetShaderResources(0, 2, textures);
			context.IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}
	}

	void DeferredRenderer::renderGBufferToBackBuffer() const
	{
		float clearColor[] = { 0, 0, 0, 0 };
		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		ID3D11ShaderResourceView* nullSRV = nullptr;

		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.OMSetDepthStencilState(m_pRenderDepthState.Get(), 0);
		context.ClearRenderTargetView(m_pRenderContext->getBackBuffer(), clearColor);

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
			ID3D11RenderTargetView* rtvs[] = { m_pRenderContext->getBackBuffer() };
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
				cbuf.position = Vec4(light.m_Position.x, light.m_Position.y, light.m_Position.z, 0.0f);
				cbuf.color    = Vec4(light.m_Color.x, light.m_Color.y, light.m_Color.z, 0.0f);
				cbuf.scale    = light.m_Radius;
				cbuf.falloff  = light.m_FaloffExponent;

				D3D11_MAPPED_SUBRESOURCE msr;
				THROW_ON_FAIL(context.Map(m_pCBPointLighting.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
				memcpy(msr.pData, (void*)&cbuf, ((sizeof(CBufferPointLight) + 15) / 16) * 16);
				context.Unmap(m_pCBPointLighting.Get(), 0);

				context.DrawIndexed(sizeof(g_SphereIndices) / sizeof(uint16_t), 0, 0);
			}
		}

		// Reset SRVs since we need them as output next frame
		for (int i = 0; i < 4; ++i) srvs[i] = nullptr;
		context.PSSetShaderResources(0, 6, srvs);
	}
}