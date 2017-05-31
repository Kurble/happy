#include "stdafx.h"
#include "RenderQueue.h"

namespace happy
{
	void RenderQueue_Root::clear()
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
		m_Quads.clear();
		m_Cones.clear();
		m_Cubes.clear();
		m_Spheres.clear();
	}

	void RenderQueue_Root::pushSkinRenderItem(const SkinRenderItem &skin)
	{
		if (skin.m_Alpha < 1.0f)
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.push_back(skin);
		else
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.push_back(skin);
	}

	void RenderQueue_Root::pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group)
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

	void RenderQueue_Root::pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group)
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

	void RenderQueue_Root::pushLight(const bb::vec3 &position, const bb::vec3 &color, float radius, float falloff)
	{
		m_PointLights.emplace_back(position, color, radius, falloff);
	}

	void RenderQueue_Root::pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter)
	{
		TextureHandle emptyHandle;
		m_Decals.emplace_back(texture, emptyHandle, transform, filter);
	}

	void RenderQueue_Root::pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Decals.emplace_back(texture, normalMap, transform, filter);
	}

	void RenderQueue_Root::pushLineWidget(const bb::vec3 &from, const bb::vec3 &to, const bb::vec4 &color)
	{
		m_Lines.emplace_back(from, to, color);
	}

	void RenderQueue_Root::pushQuadWidget(const bb::vec3 *v, const bb::vec4 &color)
	{
		m_Quads.emplace_back(v, color);
	}

	void RenderQueue_Root::pushConeWidget(const bb::vec3 &from, const bb::vec3 &to, const float radius, const bb::vec4 &color)
	{
		m_Cones.emplace_back(from, to, radius, color);
	}

	void RenderQueue_Root::pushCubeWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color)
	{
		m_Cubes.emplace_back(pos, size, color);
	}

	void RenderQueue_Root::pushSphereWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color)
	{
		m_Spheres.emplace_back(pos, size, color);
	}

	void RenderQueue_Root::pushPostProcessItem(const PostProcessItem &proc)
	{
		m_PostProcessItems.push_back(proc);
	}

	void RenderQueue::clear()
	{
		RenderQueue_Root::clear();
		for (auto &i : m_SubQueues)
		{
			i.second.clear();
		}
	}

	RenderQueue_Root& RenderQueue::asQueueForShader(const SurfaceShader& shader)
	{
		return m_SubQueues.at(shader);
	}

	void RenderQueue::setEnvironment(const PBREnvironment &environment)
	{
		m_Environment = environment;
	}

	void RenderQueue::registerShader(const SurfaceShader& shader)
	{
		if (m_SubQueues.find(shader) == m_SubQueues.end())
			m_SubQueues.emplace(shader, RenderQueue_Root());
	}
}