#pragma once

#include "TextureHandle.h"
#include "Vector.h"

namespace happy
{
	class Canvas : public TextureHandle
	{
	public:
		void clearTexture();
		void clearGeometry();

		void pushTriangleList(const vector<Vec2> &triangles);

		void renderToTexture() const;

	private:
		friend class Resources;

		void load(RenderingContext* context, unsigned width, unsigned height, DXGI_FORMAT format);

		RenderingContext *m_pContext;

		ComPtr<ID3D11RenderTargetView> m_RenderTarget;

		D3D11_VIEWPORT m_ViewPort;
		ComPtr<ID3D11RasterizerState> m_pRasterState;
		ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;
		ComPtr<ID3D11BlendState> m_pBlendState;

		ComPtr<ID3D11VertexShader> m_pVertexShader;
		ComPtr<ID3D11InputLayout> m_pInputLayout;
		ComPtr<ID3D11PixelShader> m_pPixelShader;

		ComPtr<ID3D11Buffer> m_pTriangleBuffer;
		unsigned m_TriangleBufferPtr = 0;
	};
}
