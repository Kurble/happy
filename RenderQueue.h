#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "PBREnvironment.h"
#include "SkinController.h"
#include "PostProcessItem.h"

namespace happy
{
	class RenderQueue
	{
	public:
		void clear();

		void setEnvironment(const PBREnvironment &environment);
		void pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group);
		void pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group);
		void pushSkinRenderItem(const SkinRenderItem &skin);
		void pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter);
		void pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, const bb::mat4 &transform, const StencilMask filter);
		void pushLight(const bb::vec3 &position, const bb::vec3 &color, const float radius, const float falloff);
		void pushPostProcessItem(const PostProcessItem &proc);

	private:
		friend class DeferredRenderer;

		struct MeshItem
		{
			MeshItem(const RenderMesh &mesh, const float alpha, const bb::mat4 &transform, const StencilMask group)
				: m_Mesh(mesh), m_Alpha(alpha), m_Transform(transform), m_Group(group)
			{}

			RenderMesh    m_Mesh;
			float         m_Alpha;
			bb::mat4      m_Transform;
			StencilMask   m_Group;
		};

		struct DecalItem
		{
			DecalItem(const TextureHandle &texture, const TextureHandle &normal, const bb::mat4 &transform, const StencilMask filter)
				: m_Texture(texture), m_NormalMap(normal), m_Transform(transform), m_Filter(filter)
			{}

			TextureHandle m_Texture;
			TextureHandle m_NormalMap;
			bb::mat4      m_Transform;
			StencilMask   m_Filter;
		};

		struct PointLightItem
		{
			PointLightItem(const bb::vec3 &position, const bb::vec3 &color, const float radius, const float faloff)
				: m_Position(position), m_Color(color), m_Radius(radius), m_FaloffExponent(faloff)
			{}

			bb::vec3      m_Position;
			bb::vec3      m_Color;
			float         m_Radius;
			float         m_FaloffExponent;
		};
		

		//=========================================================
		// Static geometry
		//=========================================================
		vector<MeshItem>        m_GeometryPositionTexcoord;
		vector<MeshItem>        m_GeometryPositionNormalTexcoord;
		vector<MeshItem>        m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<MeshItem>        m_GeometryPositionTexcoordTransparent;
		vector<MeshItem>        m_GeometryPositionNormalTexcoordTransparent;
		vector<MeshItem>        m_GeometryPositionNormalTangentBinormalTexcoordTransparent;

		//=========================================================
		// Dynamic geometry
		//=========================================================
		vector<SkinRenderItem>  m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<SkinRenderItem>  m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent;

		//=========================================================
		// Misc.
		//=========================================================
		PBREnvironment          m_Environment;
		vector<DecalItem>       m_Decals;
		vector<PointLightItem>  m_PointLights;
		vector<PostProcessItem> m_PostProcessItems;
	};
}
