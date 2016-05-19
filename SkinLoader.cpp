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

	RenderSkin loadSkinFromFile(RenderingContext *pRenderContext, std::string skinPath, std::string albedoMetallicPath, std::string normalRougnessPath)
	{
		std::ifstream fin(skinPath.c_str(), std::ios::in | std::ios::binary);

		uint32_t boneCount = read<uint32_t>(fin);
		vector<Mat4> bindPose;
		bindPose.reserve(boneCount);
		for (unsigned b = 0; b < boneCount; ++b)
		{
			Mat4 bind = read<Mat4>(fin);
			//bind.swapHandedness();
			bind.inverse();
			bindPose.push_back(bind);
		}

		uint32_t vertexCount = read<uint32_t>(fin);
		vector<VertexPositionNormalTangentBinormalTexcoordIndicesWeights> vertices;
		vertices.reserve(vertexCount);
		for (unsigned v = 0; v < vertexCount; ++v)
		{
			VertexPositionNormalTangentBinormalTexcoordIndicesWeights vertex;
			vertex.pos        = read<Vec4>(fin);
			vertex.normal     = read<Vec3>(fin);
			vertex.tangent    = read<Vec3>(fin);
			vertex.binormal   = read<Vec3>(fin);
			vertex.texcoord   = read<Vec2>(fin);
			vertex.indices[0] = read<Index16>(fin);
			vertex.indices[1] = read<Index16>(fin);
			vertex.indices[2] = read<Index16>(fin);
			vertex.indices[3] = read<Index16>(fin);
			vertex.weights    = read<Vec4>(fin);
			vertices.push_back(vertex);
		}

		uint32_t indexCount = read<uint32_t>(fin);
		vector<Index16> indices;
		indices.reserve(indexCount);
		for (unsigned i = 0; i < indexCount; ++i)
		{
			indices.push_back(read<Index16>(fin));
		}

		RenderSkin mesh;
		mesh.setGeometry<VertexPositionNormalTangentBinormalTexcoordIndicesWeights, Index16>(
			pRenderContext,
			vertices.data(), vertices.size(),
			indices.data(), indices.size()
		);
		mesh.setBindPose(
			pRenderContext,
			bindPose);
		if (albedoMetallicPath.length()) mesh.setAlbedoRoughnessMap(pRenderContext, loadTextureWIC(pRenderContext, albedoMetallicPath));
		if (normalRougnessPath.length()) mesh.setNormalMetallicMap(pRenderContext, loadTextureWIC(pRenderContext, normalRougnessPath));
		return mesh;
	}
}