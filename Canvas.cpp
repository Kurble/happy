#include "stdafx.h"
#include "RenderingContext.h"
#include "Resources.h"
#include "Canvas.h"

#include "CompiledShaders\CanvasVS.h"
#include "CompiledShaders\CanvasPS.h"

namespace happy
{
	static const int max_canvas_vtx_buffer_bytes = 65536;
	static const int max_canvas_vtx_buffer_vertices = max_canvas_vtx_buffer_bytes / sizeof(VertexPosition);

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
			THROW_ON_FAIL(device->CreateBlendState(&desc, &m_pBlendState));
		}

		// Shaders
		CreateVertexShader<VertexPosition>(device, m_pVertexShader, m_pInputLayout, g_shCanvasVS);
		CreatePixelShader(device, m_pPixelShader, g_shCanvasPS);

		// Dynamic vertex buffer
		{
			vector<unsigned char> vertexBuffer(65536, 0);
			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = 65536;
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

			m_TriangleBufferPtr = 0;
		}
	}

	void Canvas::clearTexture()
	{
		float clearColor[] = { 0, 0, 0, 1 };
		m_pContext->getContext()->ClearRenderTargetView(m_RenderTarget.Get(), clearColor);
	}

	void Canvas::clearGeometry()
	{
		m_TriangleBufferPtr = 0;
	}

	void Canvas::pushTriangleList(const vector<Vec2> &triangles)
	{
		if (triangles.size() % 3 || triangles.size() < 3) 
			throw std::exception("Triangle list must contain exactly 3 or a multiple of 3 vertices");

		if (m_TriangleBufferPtr > max_canvas_vtx_buffer_vertices - triangles.size())
			throw std::exception("Triange list exceeds internal buffer. Did you forget clearGeometry()?");

		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(m_pContext->getContext()->Map(m_pTriangleBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr));

		for (auto &vertex : triangles)
		{
			((VertexPosition*)msr.pData)[m_TriangleBufferPtr++] = { Vec4(vertex.x, vertex.y, 1, 1) };
		}

		m_pContext->getContext()->Unmap(m_pTriangleBuffer.Get(), 0);
	}

	void Canvas::renderToTexture() const
	{
		if (m_TriangleBufferPtr > 0)
		{
			auto &context = *m_pContext->getContext();
			context.RSSetState(m_pRasterState.Get());
			context.RSSetViewports(1, &m_ViewPort);

			context.OMSetDepthStencilState(m_pDepthStencilState.Get(), 0);
			context.OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
			context.OMSetRenderTargets(1, m_RenderTarget.GetAddressOf(), nullptr);

			context.VSSetShader(m_pVertexShader.Get(), nullptr, 0);

			context.PSSetShader(m_pPixelShader.Get(), nullptr, 0);
			UINT stride = sizeof(VertexPosition);
			UINT offset = 0;

			context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context.IASetInputLayout(m_pInputLayout.Get());
			context.IASetVertexBuffers(0, 1, m_pTriangleBuffer.GetAddressOf(), &stride, &offset);

			context.Draw(m_TriangleBufferPtr, 0);
		}
	}
}