#pragma once

#include "TextureHandle.h"
#include "PostProcessItem.h"
#include "VertexTypes.h"

namespace happy
{
	class Canvas : public TextureHandle
	{
	public:
		void clearTexture(float alpha = 1);
		void clearGeometry();

		void pushTriangleList(const VertexPositionColor *triangles, unsigned count);
		void pushTexturedTriangleList(const TextureHandle& texture, const VertexPositionColor *triangles, unsigned count);

		void runPostProcessItem(const PostProcessItem &proc) const;
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
		ComPtr<ID3D11PixelShader> m_pPixelShaderTextured;

		ComPtr<ID3D11Buffer> m_pTriangleBuffer;
		vector<unsigned> m_TriangleBufferPtrs;
		vector<ID3D11ShaderResourceView*> m_TriangleBufferTextures;
	};
}
