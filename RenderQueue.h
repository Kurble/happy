#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"
#include "PBREnvironment.h"
#include "MeshController.h"
#include "PostProcessItem.h"
#include "SurfaceShader.h"
#include "ParticleBuilder.h"

namespace happy
{
	class RenderQueue_Root
	{
	public:
		virtual void clear();
		bool empty() const;

		void pushRenderMesh(const RenderMesh &mesh, const bb::mat4 &transform, const StencilMask group);
		void pushRenderMesh(const RenderMesh &mesh, float alpha, const bb::mat4 &transform, const StencilMask group);
		void pushSkinRenderItem(const SkinRenderItem &skin);
		void pushDecal(const TextureHandle &texture, const bb::mat4 &transform, const StencilMask filter);
		void pushDecal(const TextureHandle &texture, float alpha, const bb::mat4 &transform, const StencilMask filter);
		void pushDecal(const TextureHandle &texture, const TextureHandle &normalMap, float alpha, const bb::mat4 &transform, const StencilMask filter);
		void pushLineWidget(const bb::vec3 &from, const bb::vec3 &to, const bb::vec4 &color);
		void pushQuadWidget(const bb::vec3 *v, const bb::vec4 &color);
		void pushConeWidget(const bb::vec3 &from, const bb::vec3 &to, const float radius, const bb::vec4 &color);
		void pushCubeWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color);
		void pushSphereWidget(const bb::vec3 &pos, const float size, const bb::vec4 &color);
		void pushLight(const bb::vec3 &position, const bb::vec3 &color, const float radius, const float falloff);
		void pushPostProcessItem(const PostProcessItem &proc);
		void pushNewParticle(const VertexParticle &particle);

	protected:
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
			DecalItem(const TextureHandle &texture, const TextureHandle &normal, float alpha, const bb::mat4 &transform, const StencilMask filter)
				: m_Texture(texture), m_NormalMap(normal), m_Alpha(alpha), m_Transform(transform), m_Filter(filter)
			{}

			TextureHandle m_Texture;
			TextureHandle m_NormalMap;
			float         m_Alpha;
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

		struct LineWidgetItem
		{
			LineWidgetItem(const bb::vec3 &from, const bb::vec3 &to, const bb::vec4 &color)
				: m_From(from), m_To(to), m_Color(color)
			{}

			bb::vec4      m_From;
			bb::vec4      m_To;
			bb::vec4      m_Color;
		};

		struct QuadWidgetItem
		{
			QuadWidgetItem(const bb::vec3 *v, const bb::vec4 &color)
				: m_V{ v[0], v[1], v[2], v[3] }, m_Color(color)
			{}

			bb::vec4      m_V[4];
			bb::vec4      m_Color;
		};

		struct ConeWidgetItem
		{
			ConeWidgetItem(const bb::vec3 &from, const bb::vec3 &to, float radius, const bb::vec4 &color)
				: m_From(from.x, from.y, from.z, 1.0f), m_To(to.x, to.y, to.z, 1.0f), m_Radius(radius), m_Color(color)
			{}

			bb::vec4      m_From;
			bb::vec4      m_To;
			float         m_Radius;
			bb::vec4      m_Color;
		};

		struct SimpleWidgetItem
		{
			SimpleWidgetItem(const bb::vec3 &pos, float size, const bb::vec4 &color)
				: m_Pos(pos), m_Size(size), m_Color(color)
			{}

			bb::vec4      m_Pos;
			float         m_Size;
			bb::vec4      m_Color;
		};

		bool                     m_Empty = true;

		//=========================================================
		// Static geometry
		//=========================================================
		vector<MeshItem>         m_GeometryPositionTexcoord;
		vector<MeshItem>         m_GeometryPositionNormalTexcoord;
		vector<MeshItem>         m_GeometryPositionNormalTangentBinormalTexcoord;
		vector<MeshItem>         m_GeometryPositionTexcoordTransparent;
		vector<MeshItem>         m_GeometryPositionNormalTexcoordTransparent;
		vector<MeshItem>         m_GeometryPositionNormalTangentBinormalTexcoordTransparent;

		//=========================================================
		// Dynamic geometry
		//=========================================================
		vector<SkinRenderItem>   m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights;
		vector<SkinRenderItem>   m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent;

		//=========================================================
		// Widgets
		//=========================================================
		vector<LineWidgetItem>   m_Lines;
		vector<QuadWidgetItem>   m_Quads;
		vector<ConeWidgetItem>   m_Cones;
		vector<SimpleWidgetItem> m_Cubes;
		vector<SimpleWidgetItem> m_Spheres;

		//=========================================================
		// Misc.
		//=========================================================
		vector<DecalItem>        m_Decals;
		vector<VertexParticle>   m_Particles;
		vector<PointLightItem>   m_PointLights;
		vector<PostProcessItem>  m_PostProcessItems;
	};

	class RenderQueue : public RenderQueue_Root
	{
	public:
		RenderQueue_Root& asQueueForShader(const SurfaceShader& shader);

		void clear();

		void setEnvironment(const PBREnvironment &environment);

		void setParticleAtlas(const TextureHandle &particleAtlas);

	private:
		friend class DeferredRenderer;

		map<SurfaceShader, RenderQueue_Root> m_SubQueues;

		PBREnvironment m_Environment;

		TextureHandle m_ParticleAtlas;
	};
}
