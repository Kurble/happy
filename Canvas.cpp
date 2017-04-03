#include "stdafx.h"
#include "RenderingContext.h"
#include "Resources.h"
#include "Canvas.h"

#include "CompiledShaders\CanvasVS.h"
#include "CompiledShaders\CanvasPS.h"
#include "CompiledShaders\CanvasTexPS.h"

namespace happy
{
	static const int max_canvas_vtx_buffer_bytes = 1048576; // 1 MiB
	static const int max_canvas_vtx_buffer_vertices = max_canvas_vtx_buffer_bytes / sizeof(VertexPositionColor);

	void Canvas::load(RenderingContext *context, unsigned width, unsigned height, DXGI_FORMAT format)
	{
		m_pContext = context;

		auto device = m_pContext->getDevice();

		//-------------------------------------------------------------
		// Create RTT handle
		{
			vector<unsigned char> texture(width * height * 4, 0x80);
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Format = format;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = texture.data();
			data.SysMemPitch = width * 4;
			data.SysMemSlicePitch = 0;

			ComPtr<ID3D11Texture2D> textureHandle;
			THROW_ON_FAIL(device->CreateTexture2D(&desc, &data, &textureHandle));
			THROW_ON_FAIL(device->CreateShaderResourceView(textureHandle.Get(), nullptr, m_Handle.GetAddressOf()));
			THROW_ON_FAIL(device->CreateRenderTargetView(textureHandle.Get(), nullptr, m_RenderTarget.GetAddressOf()));
		}

		//-------------------------------------------------------------
		// Setup rendering state

		// Viewport
		m_ViewPort.Width = (float)width;
		m_ViewPort.Height = (float)height;
		m_ViewPort.MinDepth = 0.0f;
		m_ViewPort.MaxDepth = 1.0f;
		m_ViewPort.TopLeftX = 0;
		m_ViewPort.TopLeftY = 0;

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
			THROW_ON_FAIL(device->CreateRasterizerState(&desc, &m_pRasterState));
		}

		// Depth stencil state
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = false;
			desc.StencilEnable = false;
			THROW_ON_FAIL(device->CreateDepthStencilState(&desc, &m_pDepthStencilState));
		}

		// Blending state
		{
			D3D11_BLEND_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask =
				D3D11_COLOR_WRITE_ENABLE_RED |
				D3D11_COLOR_WRITE_ENABLE_GREEN |
				D3D11_COLOR_WRITE_ENABLE_BLUE |
				D3D11_COLOR_WRITE_ENABLE_ALPHA;
			desc.IndependentBlendEnable = false;
			THROW_ON_FAIL(device->CreateBlendState(&desc, &m_pBlendState));
		}

		// Shaders
		CreateVertexShader<VertexPositionColor>(device, m_pVertexShader, m_pInputLayout, g_shCanvasVS);
		CreatePixelShader(device, m_pPixelShader, g_shCanvasPS);
		CreatePixelShader(device, m_pPixelShaderTextured, g_shCanvasTexPS);

		// Dynamic vertex buffer
		{
			vector<unsigned char> vertexBuffer(max_canvas_vtx_buffer_bytes, 0);
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = max_canvas_vtx_buffer_bytes;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = vertexBuffer.data();
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			THROW_ON_FAIL(device->CreateBuffer(&desc, &data, m_pTriangleBuffer.GetAddressOf()));

			m_TriangleBufferPtrs.clear();
			m_TriangleBufferPtrs.push_back(0);
			m_TriangleBufferTextures.clear();
			m_TriangleBufferTextures.push_back(nullptr);
		}
	}

	void Canvas::clearTexture(float alpha)
	{
		float clearColor[] = { 0, 0, 0, alpha };
		m_pContext->getContext()->ClearRenderTargetView(m_RenderTarget.Get(), clearColor);
	}

	void Canvas::clearGeometry()
	{
		m_TriangleBufferPtrs.clear();
		m_TriangleBufferPtrs.push_back(0);
		m_TriangleBufferTextures.clear();
		m_TriangleBufferTextures.push_back(nullptr);
	}

	void Canvas::pushTriangleList(const VertexPositionColor *triangles, const unsigned count)
	{
		if (count % 3 || count < 3)
			throw std::exception("Triangle list must contain exactly 3 or a multiple of 3 vertices");

		if (m_TriangleBufferPtrs.back() > max_canvas_vtx_buffer_vertices - count)
			throw std::exception("Triange list exceeds internal buffer. Did you forget clearGeometry()?");

		if (m_TriangleBufferTextures.back() != nullptr)
		{
			m_TriangleBufferPtrs.push_back(m_TriangleBufferPtrs.back());
			m_TriangleBufferTextures.push_back(nullptr);
		}

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(m_pContext->getContext()->Map(m_pTriangleBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr));

		memcpy(&((VertexPositionColor*)msr.pData)[m_TriangleBufferPtrs.back()], triangles, count * sizeof(VertexPositionColor));
		m_TriangleBufferPtrs.back() += count;

		m_pContext->getContext()->Unmap(m_pTriangleBuffer.Get(), 0);
	}

	void Canvas::pushTexturedTriangleList(const TextureHandle &texture, const VertexPositionColor *triangles, const unsigned count)
	{
		if (count % 3 || count < 3)
			throw std::exception("Triangle list must contain exactly 3 or a multiple of 3 vertices");

		if (m_TriangleBufferPtrs.back() > max_canvas_vtx_buffer_vertices - count)
			throw std::exception("Triange list exceeds internal buffer. Did you forget clearGeometry()?");

		if (m_TriangleBufferTextures.back() != texture.m_Handle.Get())
		{
			m_TriangleBufferPtrs.push_back(m_TriangleBufferPtrs.back());
			m_TriangleBufferTextures.push_back(texture.m_Handle.Get());
		}

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(m_pContext->getContext()->Map(m_pTriangleBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr));

		memcpy(&((VertexPositionColor*)msr.pData)[m_TriangleBufferPtrs.back()], triangles, count * sizeof(VertexPositionColor));
		m_TriangleBufferPtrs.back() += count;

		m_pContext->getContext()->Unmap(m_pTriangleBuffer.Get(), 0);
	}

	void Canvas::runPostProcessItem(const PostProcessItem &process) const
	{
		static const VertexPositionColor quad[] =
		{
			{ { -1,  1, 0, 1 }, { 0, 0, 0, 0 } },
			{ {  1,  1, 0, 1 }, { 1, 0, 0, 0 } },
			{ {  1, -1, 0, 1 }, { 1, 1, 0, 0 } },
			{ { -1,  1, 0, 1 }, { 0, 0, 0, 0 } },
			{ {  1, -1, 0, 1 }, { 1, 1, 0, 0 } },
			{ { -1, -1, 0, 1 }, { 0, 1, 0, 0 } },
		};

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(m_pContext->getContext()->Map(m_pTriangleBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr));
		memcpy(&((VertexPositionColor*)msr.pData)[m_TriangleBufferPtrs.back()], quad, 6 * sizeof(VertexPositionColor));
		m_pContext->getContext()->Unmap(m_pTriangleBuffer.Get(), 0);

		auto &context = *m_pContext->getContext();
		context.RSSetState(m_pRasterState.Get());
		context.RSSetViewports(1, &m_ViewPort);

		context.OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context.OMSetRenderTargets(1, m_RenderTarget.GetAddressOf(), nullptr);

		context.VSSetShader(m_pVertexShader.Get(), nullptr, 0);

		ID3D11Buffer* constBuffers[2] = { 0 };

		ID3D11ShaderResourceView* pp_srvs[10] = { 0 };
		for (auto &slot : process.m_InputSlots)
			pp_srvs[slot.first] = (ID3D11ShaderResourceView*)slot.second;

		if (process.m_ConstBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE msr;
			THROW_ON_FAIL(context.Map(process.m_ConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
			memcpy(msr.pData, (void*)process.m_ConstBufferData->data(), process.m_ConstBufferData->size());
			context.Unmap(process.m_ConstBuffer.Get(), 0);

			constBuffers[1] = process.m_ConstBuffer.Get();
		}
		else
		{
			constBuffers[1] = nullptr;
		}

		context.PSSetShader(process.m_Handle.Get(), nullptr, 0);
		context.PSSetShaderResources(0, 10, pp_srvs);
		context.PSSetConstantBuffers(0, 2, constBuffers);

		UINT stride = sizeof(VertexPositionColor);
		UINT offset = 0;
		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(m_pInputLayout.Get());
		context.IASetVertexBuffers(0, 1, m_pTriangleBuffer.GetAddressOf(), &stride, &offset);
		context.Draw(6, m_TriangleBufferPtrs.back());

		ID3D11ShaderResourceView* reset_srvs[10] = { 0 };
		context.PSSetShaderResources(0, 10, reset_srvs);
	}

	void Canvas::renderToTexture() const
	{
		auto &context = *m_pContext->getContext();
		context.RSSetState(m_pRasterState.Get());
		context.RSSetViewports(1, &m_ViewPort);

		context.OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
		context.OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
		context.OMSetRenderTargets(1, m_RenderTarget.GetAddressOf(), nullptr);

		context.VSSetShader(m_pVertexShader.Get(), nullptr, 0);

		UINT stride = sizeof(VertexPositionColor);
		UINT offset = 0;

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(m_pInputLayout.Get());
		context.IASetVertexBuffers(0, 1, m_pTriangleBuffer.GetAddressOf(), &stride, &offset);

		unsigned prev = 0;
		for (unsigned i = 0; i < m_TriangleBufferPtrs.size(); ++i)
		{
			if (m_TriangleBufferPtrs[i]-prev > 0)
			{
				if (m_TriangleBufferTextures[i])
				{
					context.PSSetShader(m_pPixelShaderTextured.Get(), nullptr, 0);
					context.PSSetShaderResources(0, 1, &m_TriangleBufferTextures[i]);
				}
				else
				{
					context.PSSetShader(m_pPixelShader.Get(), nullptr, 0);
				}
				
				context.Draw(m_TriangleBufferPtrs[i] - prev, prev);
			}
			prev = m_TriangleBufferPtrs[i];
		}

		ID3D11ShaderResourceView* reset = nullptr;
		context.PSSetShaderResources(0, 1, &reset);
	}
}