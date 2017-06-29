#include "stdafx.h"
#include "RenderQueue.h"

namespace happy
{
	void RenderQueue_Root::clear()
	{
		m_Empty = true;
		m_GeometryPositionTexcoord.clear();
		m_GeometryPositionNormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoord.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.clear();
		m_GeometryPositionTexcoordTransparent.clear();
		m_GeometryPositionNormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordTransparent.clear();
		m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.clear();
		m_Decals.clear();
		m_Particles.clear();
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
		m_Empty = false;

		if (skin.m_Color.w < 1.0f)
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.push_back(skin);
		else
			m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.push_back(skin);
	}

	void RenderQueue_Root::pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group)
	{
		m_Empty = false;

		switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoord.emplace_back(mesh, bb::vec4(1, 1, 1, 1), transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoord.emplace_back(mesh, bb::vec4(1, 1, 1, 1), transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoord.emplace_back(mesh, bb::vec4(1, 1, 1, 1), transform, group);
			break;
		}
	}

	void RenderQueue_Root::pushRenderMesh(const RenderMesh &mesh, const bb::vec4& color, const bb::mat4 &transform, const StencilMask group)
	{
		m_Empty = false;

		if (color.w >= 1.0f) switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoord.emplace_back(mesh, color, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoord.emplace_back(mesh, color, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoord.emplace_back(mesh, color, transform, group);
			break;
		}
		else switch (mesh.getVertexType())
		{
		case VertexType::VertexPositionTexcoord:
			m_GeometryPositionTexcoordTransparent.emplace_back(mesh, color, transform, group);
			break;
		case VertexType::VertexPositionNormalTexcoord:
			m_GeometryPositionNormalTexcoordTransparent.emplace_back(mesh, color, transform, group);
			break;
		case VertexType::VertexPositionNormalTangentBinormalTexcoord:
			m_GeometryPositionNormalTangentBinormalTexcoordTransparent.emplace_back(mesh, color, transform, group);
			break;
		}
	}

	void RenderQueue_Root::pushLight(const bb::vec3 &position, const bb::vec3 &color, float radius, float falloff)
	{
		m_Empty = false;

		m_PointLights.emplace_back(position, color, radius, falloff);
	}

	void RenderQueue_Root::pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Empty = false;

		TextureHandle emptyHandle;
		m_Decals.emplace_back(texture, emptyHandle, bb::vec4(1, 1, 1, 1), transform, filter);
	}

	void RenderQueue_Root::pushDecal(const TextureHandle &texture, const bb::vec4& color, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Empty = false;

		TextureHandle emptyHandle;
		m_Decals.emplace_back(texture, emptyHandle, color, transform, filter);
	}

	void RenderQueue_Root::pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::vec4& color, const bb::mat4 &transform, const StencilMask filter)
	{
		m_Empty = false;

		m_Decals.emplace_back(texture, normalMap, color, transform, filter);
	}

	void RenderQueue_Root::pushNewParticle(const VertexParticle& particle)
	{
		m_Empty = false;

		m_Particles.push_back(particle);
	}

	void RenderQueue_Root::pushLineWidget(const bb::vec3 &from, const bb::vec3 &to, const bb::vec4 &color)
	{
		m_Empty = false;

		m_Lines.emplace_back(from, to, color);
	}

	void RenderQueue_Root::pushQuadWidget(const bb::vec3 *v, const bb::vec4 &color)
	{
		m_Empty = false;

		m_Quads.emplace_back(v, color);
	}

	void RenderQueue_Root::pushConeWidget(const bb::vec3 &from, const bb::vec3 &to, const float radius, const bb::vec4 &color)
	{
		m_Empty = false;

		m_Cones.emplace_back(from, to, radius, color);
	}

	void RenderQueue_Root::pushCubeWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color)
	{
		m_Empty = false;

		m_Cubes.emplace_back(pos, size, color);
	}

	void RenderQueue_Root::pushSphereWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color)
	{
		m_Empty = false;

		m_Spheres.emplace_back(pos, size, color);
	}

	void RenderQueue_Root::pushPostProcessItem(const PostProcessItem &proc)
	{
		m_Empty = false;

		m_PostProcessItems.push_back(proc);
	}

	bool RenderQueue_Root::empty() const
	{
		return m_Empty;
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
		if (m_SubQueues.find(shader) == m_SubQueues.end())
			m_SubQueues.emplace(shader, RenderQueue_Root());
		return m_SubQueues.at(shader);
	}

	void RenderQueue::setEnvironment(const PBREnvironment &environment)
	{
		m_Environment = environment;
	}

	void RenderQueue::setParticleAtlas(const TextureHandle& particleAtlas)
	{
		m_ParticleAtlas = particleAtlas;
	}
}