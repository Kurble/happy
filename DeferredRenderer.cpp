#include "stdafx.h"
#include "DeferredRenderer.h"

//----------------------------------------------------------------------
// G-Buffer shaders
#include "CompiledShaders\VertexPositionTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTexcoord.h"
#include "CompiledShaders\VertexPositionNormalTangentBinormalTexcoord.h"
#include "CompiledShaders\GeometryPS.h"
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Rendering shaders
#include "CompiledShaders\ScreenQuadVS.h"
#include "CompiledShaders\DeferredDirectionalOcclusion.h"
#include "CompiledShaders\EnvironmentalLighting.h"
//----------------------------------------------------------------------

namespace happy
{
	struct CBufferScene
	{
		Mat4 view;
		Mat4 projection;
		Mat4 viewInverse;
		Mat4 projectionInverse;
		float width;
		float height;
		unsigned int convolutionStages;
	};

	struct CBufferObject
	{
		Mat4 world;
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
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
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

		// G-Buffer shaders
		CreateVertexShader<VertexPositionTexcoord>(m_pVSPositionTexcoord, m_pILPositionTexcoord, g_shVertexPositionTexcoord);
		CreateVertexShader<VertexPositionNormalTexcoord>(m_pVSPositionNormalTexcoord, m_pILPositionNormalTexcoord, g_shVertexPositionNormalTexcoord);
		CreateVertexShader<VertexPositionNormalTangentBinormalTexcoord>(m_pVSPositionNormalTangentBinormalTexcoord, m_pILPositionNormalTangentBinormalTexcoord, g_shVertexPositionNormalTangentBinormalTexcoord);
		CreatePixelShader(m_pPSGeometry, g_shGeometryPS);
		
		// Rendering shaders
		CreateVertexShader<VertexPositionTexcoord>(m_pVSScreenQuad, m_pILScreenQuad, g_shScreenQuadVS);
		CreatePixelShader(m_pPSDSSDO, g_shDeferredDirectionalOcclusion);
		CreatePixelShader(m_pPSGlobalLighting, g_shEnvironmentalLighting);

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
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
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

		for (unsigned int i = 0; i < 3; ++i)
		{
			THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &m_pGBuffer[i]));
			THROW_ON_FAIL(device.CreateRenderTargetView(m_pGBuffer[i].Get(), nullptr, m_pGBufferTarget[i].GetAddressOf()));
			THROW_ON_FAIL(device.CreateShaderResourceView(m_pGBuffer[i].Get(), nullptr, m_pGBufferView[i].GetAddressOf()));
		}

		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		THROW_ON_FAIL(device.CreateTexture2D(&texDesc, &data, &m_pGBuffer[3]));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		dsvDesc.Flags = 0;
		THROW_ON_FAIL(device.CreateDepthStencilView(m_pGBuffer[3].Get(), &dsvDesc, m_pDepthBufferView.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		THROW_ON_FAIL(device.CreateShaderResourceView(m_pGBuffer[3].Get(), &srvDesc, m_pGBufferView[3].GetAddressOf()));
	}

	void DeferredRenderer::clear()
	{
		m_GeometryPositionTexcoord.clear();
		m_GeometryPositionNormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoord.clear();

		m_PointLights.clear();
	}

	void DeferredRenderer::pushRenderMesh(RenderMesh &mesh, Mat4 &transform)
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

	void DeferredRenderer::pushLight(Vec3 position, Vec3 color, float radius, float falloff)
	{
		PointLight pl = { position, color, radius, falloff };
		m_PointLights.emplace_back(pl);
	}

	void DeferredRenderer::setEnvironment(PBREnvironment &environment)
	{
		m_Environment = environment;
	}

	void DeferredRenderer::setCamera(Mat4 view, Mat4 projection)
	{
		m_View = view;
		m_Projection = projection;
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

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(context.Map(m_pCBScene.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, (void*)&sceneCB, ((sizeof(CBufferScene) + 15) / 16) * 16);
		context.Unmap(m_pCBScene.Get(), 0);

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
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferScene) + 15) / 16) * 16);
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
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferScene) + 15) / 16) * 16);
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
			memcpy(msr.pData, (void*)&objectCB, ((sizeof(CBufferScene) + 15) / 16) * 16);
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
	}

	void DeferredRenderer::renderGBufferToBackBuffer() const
	{
		float clearColor[] = { 0, 0, 0, 0 };

		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.OMSetDepthStencilState(m_pRenderDepthState.Get(), 0);
		context.ClearRenderTargetView(m_pRenderContext->getBackBuffer(), clearColor);

		ID3D11SamplerState* samplers[] = { m_pScreenSampler.Get(), m_pGSampler.Get() };

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(m_pILScreenQuad.Get());
		context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
		context.PSSetSamplers(0, 2, samplers);
		context.PSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		UINT stride = sizeof(VertexPositionTexcoord);
		UINT offset = 0;
		context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);
		
		//--------------------------------------------------------------------
		// Generate DSSDO buffer
		{
			context.OMSetRenderTargets(1, m_pGBufferTarget[2].GetAddressOf(), nullptr);
			context.PSSetShader(m_pPSDSSDO.Get(), nullptr, 0);
			ID3D11ShaderResourceView* srvs[] = { m_pGBufferView[0].Get(), m_pGBufferView[1].Get(), nullptr, m_pGBufferView[3].Get(), m_pNoiseTexture.Get(), nullptr };
			context.PSSetShaderResources(0, 5, srvs);

			context.Draw(6, 0);
		}

		//--------------------------------------------------------------------
		// Render environmental lighting
		{
			ID3D11RenderTargetView* rtvs[] = { m_pRenderContext->getBackBuffer() };
			context.OMSetRenderTargets(1, rtvs, nullptr);
			context.OMSetBlendState(m_pRenderBlendState.Get(), nullptr, 0xffffffff);
			context.PSSetShader(m_pPSGlobalLighting.Get(), nullptr, 0);
			ID3D11ShaderResourceView* srvs[] = { m_pGBufferView[0].Get(), m_pGBufferView[1].Get(), m_pGBufferView[2].Get(), m_pGBufferView[3].Get(), nullptr, nullptr };
			context.PSSetShaderResources(0, 4, srvs);
			context.PSSetShaderResources(4, 1, m_Environment.getLightingSRV());
			context.PSSetShaderResources(5, 1, m_Environment.getEnvironmentSRV());

			context.Draw(6, 0);

			for (int i = 0; i < 4; ++i) srvs[i] = nullptr;
			context.PSSetShaderResources(0, 6, srvs);
		}
	}
}