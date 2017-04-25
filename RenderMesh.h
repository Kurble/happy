#pragma once

#include "RenderingContext.h"
#include "VertexTypes.h"
#include "MultiTexture.h"

namespace happy
{
	class RenderMesh
	{
	public:
		virtual ~RenderMesh() { }

		virtual shared_ptr<RenderMesh> clone() const { return make_shared<RenderMesh>(*this); }

		template <typename Vtx, typename Idx> void setGeometry(const RenderingContext *pRenderContext, Vtx* vertices, size_t vtxCount, Idx* indices, size_t idxCount)
		{
			m_IndexCount = idxCount;
			m_VertexType = Vtx::Type;
			m_VertexStride = sizeof(Vtx);

			D3D11_BUFFER_DESC vtxDesc, idxDesc;
			ZeroMemory(&vtxDesc, sizeof(vtxDesc));
			ZeroMemory(&idxDesc, sizeof(vtxDesc));

			vtxDesc.ByteWidth = (UINT)vtxCount * sizeof(Vtx);
			vtxDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vtxDesc.Usage = D3D11_USAGE_IMMUTABLE;
			D3D11_SUBRESOURCE_DATA vtxData = { 0 };
			vtxData.pSysMem = vertices;

			idxDesc.ByteWidth = (UINT)idxCount * sizeof(Idx);
			idxDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			idxDesc.Usage = D3D11_USAGE_IMMUTABLE;
			D3D11_SUBRESOURCE_DATA idxData = { 0 };
			idxData.pSysMem = indices;

			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&vtxDesc, &vtxData, m_pVtx.GetAddressOf()));
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&idxDesc, &idxData, m_pIdx.GetAddressOf()));
		}
		
		void setMultiTexture(MultiTexture &texture);

		VertexType getVertexType() const;
		ID3D11Buffer* getVtxBuffer() const;
		ID3D11Buffer* getIdxBuffer() const;
		size_t getIndexCount() const;
		size_t getVertexStride() const;
		ID3D11ShaderResourceView** getTextures() const;

	private:
		friend class Resources;

		VertexType m_VertexType;
		size_t m_VertexStride;
		ComPtr<ID3D11Buffer> m_pVtx;
		ComPtr<ID3D11Buffer> m_pIdx;
		size_t m_IndexCount;
		MultiTexture m_Textures;
	};
}