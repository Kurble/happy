#include "stdafx.h"
#include "RenderMesh.h"

namespace happy
{
	void RenderMesh::setMultiTexture(MultiTexture &texture)
	{
		m_Textures = texture;
	}

	VertexType RenderMesh::getVertexType() const
	{
		return m_VertexType;
	}

	size_t RenderMesh::getVertexStride() const
	{
		return m_VertexStride;
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

	ID3D11ShaderResourceView** RenderMesh::getTextures() const
	{
		return m_Textures.getTextures();
	}
}