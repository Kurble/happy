#include "stdafx.h"
#include "RenderQueue.h"

namespace happy
{
	void RenderQueue::clear()
	{
		m_GeometryPositionTexcoord.clear();
		m_GeometryPositionNormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.clear();
		m_GeometryPositionTexcoordTransparent.clear();
		m_GeometryPositionNormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.clear();
		m_Decals.clear();
		m_PointLights.clear();
		m_PostProcessItems.clear();
		m_Lines.clear();
	}

	void RenderQueue::pushSkinRenderItem(const SkinRenderItem &skin)
	{
		if (skin.m_Alpha < 1.0f)
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.push_back(skin);
		else
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.push_back(skin);
	}

	void RenderQueue::pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group)
	{
		switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoord.emplace_back(mesh, 1.0f, transform, group);
			break;
		}
	}

	void RenderQueue::pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group)
	{
		if (alpha >= 1.0f) pushRenderMesh(mesh, transform, group);
		else switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoordTransparent.emplace_back(mesh, alpha, transform, group);
			break;
		}
	}

	void RenderQueue::pushLight(const bb::vec3 &position, const bb::vec3 &color, float radius, float falloff)
	{
		m_PointLights.emplace_back(position, color, radius, falloff);
	}

	void RenderQueue::pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter)
	{
		TextureHandle emptyHandle;
		m_Decals.emplace_back(texture, emptyHandle, transform, filter);
	}

	void RenderQueue::pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Decals.emplace_back(texture, normalMap, transform, filter);
	}

	void RenderQueue::pushLineWidget(const bb::vec3 &from, const bb::vec3 &to, const bb::vec4 &color)
	{
		m_Lines.emplace_back(from, to, color);
	}

	void RenderQueue::pushPostProcessItem(const PostProcessItem &proc)
	{
		m_PostProcessItems.push_back(proc);
	}

	void RenderQueue::setEnvironment(const PBREnvironment &environment)
	{
		m_Environment = environment;
	}
}