#include "stdafx.h"
#include "AssetLoaders.h"

namespace happy
{
	template <typename T> T read(istream &s)
	{
		T val;
		s.read(reinterpret_cast<char*>(&val), sizeof(T));
		return val;
	}

	template<> VertexPositionNormalTangentBinormalTexcoordIndicesWeights read(istream &s)
	{
		VertexPositionNormalTangentBinormalTexcoordIndicesWeights vertex;
		vertex.pos = read<bb::vec4>(s);
		vertex.normal = read<bb::vec3>(s);
		vertex.tangent = read<bb::vec3>(s);
		vertex.binormal = read<bb::vec3>(s);
		vertex.texcoord = read<bb::vec2>(s);
		vertex.indices[0] = read<Index16>(s);
		vertex.indices[1] = read<Index16>(s);
		vertex.indices[2] = read<Index16>(s);
		vertex.indices[3] = read<Index16>(s);
		vertex.weights = read<bb::vec4>(s);
		vertex.weights = vertex.weights * (1.0f / (vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w));
		return vertex;
	}

	template<> VertexPositionNormalTangentBinormalTexcoord read(istream &s)
	{
		VertexPositionNormalTangentBinormalTexcoord vertex;
		vertex.pos = read<bb::vec4>(s);
		vertex.normal = read<bb::vec3>(s);
		vertex.tangent = read<bb::vec3>(s);
		vertex.binormal = read<bb::vec3>(s);
		vertex.texcoord = read<bb::vec2>(s);
		return vertex;
	}

	template <typename M, typename V, typename I>
	void loadGeometry(RenderingContext* context, M* mesh, std::ifstream &fin)
	{
		uint32_t vertexCount = read<uint32_t>(fin);
		vector<V> vertices;
		vertices.reserve(vertexCount);
		for (unsigned v = 0; v < vertexCount; ++v)
		{
			vertices.push_back(read<V>(fin));
		}

		uint32_t indexCount = read<uint32_t>(fin);
		vector<I> indices;
		indices.reserve(indexCount);
		for (unsigned i = 0; i < indexCount; ++i)
		{
			indices.push_back(read<I>(fin));
		}

		mesh->setGeometry(context, vertices.data(), vertices.size(), indices.data(), indices.size());
	}

	shared_ptr<RenderMesh> loadRenderMeshFromHappyFile(RenderingContext *pRenderContext, fs::path filePath)
	{
		std::ifstream fin(filePath.c_str(), std::ios::in | std::ios::binary);
		
		uint32_t version = read<uint32_t>(fin);
		version;
		uint32_t type = read<uint32_t>(fin);

		switch (type)
		{
		case 0: // static mesh
		{
			RenderMesh mesh;

			//-------------------------------
			// Geometry
			loadGeometry<RenderMesh, VertexPositionNormalTangentBinormalTexcoord, Index16>(pRenderContext, &mesh, fin);

			return make_shared<RenderMesh>(mesh);
		}
		break;

		case 1: // skin mesh
		{
			RenderSkin mesh;

			//-------------------------------
			// Bind pose
			uint32_t boneCount = read<uint32_t>(fin);
			vector<bb::mat4> bindPose;
			bindPose.reserve(boneCount);
			for (unsigned b = 0; b < boneCount; ++b)
			{
				bb::mat4 bind = read<bb::mat4>(fin);
				bind.inverse();
				bindPose.push_back(bind);
			}
			mesh.setBindPose(pRenderContext, bindPose);

			//-------------------------------
			// Geometry
			loadGeometry<RenderSkin, VertexPositionNormalTangentBinormalTexcoordIndicesWeights, Index16>(pRenderContext, &mesh, fin);
			
			return make_shared<RenderSkin>(mesh);
		}
		break;

		default:
		{
			throw std::exception("invalid mesh type found in .happy file");
		}
		break;
		}
	}
}