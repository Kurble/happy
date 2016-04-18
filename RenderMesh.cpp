#include "stdafx.h"
#include "RenderMesh.h"

namespace happy
{
	void RenderMesh::setAlbedoRoughnessMap(const RenderingContext *pRenderContext, ComPtr<ID3D11ShaderResourceView>& texture)
	{
		m_pAlbedoRoughness = texture;
	}

	void RenderMesh::setNormalMetallicMap(const RenderingContext *pRenderContext, ComPtr<ID3D11ShaderResourceView>& texture)
	{
		m_pNormalMetallic = texture;
	}

	VertexType RenderMesh::getVertexType() const
	{
		return m_VertexType;
	}

	ID3D11Buffer* RenderMesh::getVtxBuffer() const
	{
		return m_pVtx.Get();
	}

	ID3D11Buffer* RenderMesh::getIdxBuffer() const
	{
		return m_pIdx.Get();
	}

	size_t RenderMesh::getIndexCount() const
	{
		return m_IndexCount;
	}

	ID3D11ShaderResourceView * RenderMesh::getAlbedoRoughnessMap() const
	{
		return m_pAlbedoRoughness.Get();
	}

	ID3D11ShaderResourceView * RenderMesh::getNormalMetallicMap() const
	{
		return m_pNormalMetallic.Get();
	}
}