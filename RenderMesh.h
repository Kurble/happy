#pragma once

#include "RenderingContext.h"
#include "VertexTypes.h"

namespace happy
{
	class RenderMesh
	{
	public:
		template <typename Vtx, typename Idx> void setGeometry(const RenderingContext *pRenderContext, Vtx* vertices, size_t vtxCount, Idx* indices, size_t idxCount)
		{
			m_IndexCount = idxCount;
			m_VertexType = Vtx::Type;

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
		
		void setAlbedoRoughnessMap(const RenderingContext *pRenderContext, ComPtr<ID3D11ShaderResourceView> &texture);
		void setNormalMetallicMap(const RenderingContext *pRenderContext, ComPtr<ID3D11ShaderResourceView> &texture);

		VertexType getVertexType() const;
		ID3D11Buffer* getVtxBuffer() const;
		ID3D11Buffer* getIdxBuffer() const;
		size_t getIndexCount() const;
		ID3D11ShaderResourceView* getAlbedoRoughnessMap() const;
		ID3D11ShaderResourceView* getNormalMetallicMap() const;

	private:
		friend class Resources;

		VertexType m_VertexType;
		ComPtr<ID3D11Buffer> m_pVtx;
		ComPtr<ID3D11Buffer> m_pIdx;
		size_t m_IndexCount;
		ComPtr<ID3D11ShaderResourceView> m_pAlbedoRoughness;
		ComPtr<ID3D11ShaderResourceView> m_pNormalMetallic;
	};
}