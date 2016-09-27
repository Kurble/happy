#pragma once

#include <inttypes.h>

#include "Vector.h"
#include "Matrix.h"

namespace happy
{
	enum class VertexType
	{
		VertexPosition,
		VertexPositionTexcoord,
		VertexPositionNormalTexcoord,
		VertexPositionNormalTangentBinormalTexcoord,
		VertexPositionNormalTangentBinormalTexcoordIndicesWeights,
	};

	typedef uint16_t Index16;
	typedef uint32_t Index32;

	struct VertexPosition
	{
		Vec4 pos;

		static const D3D11_INPUT_ELEMENT_DESC Elements[1];
		static const UINT ElementCount = 1;
		static const VertexType Type = VertexType::VertexPosition;
	};

	struct VertexPositionTexcoord
	{
		Vec4 pos;
		Vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[2];
		static const UINT ElementCount = 2;
		static const VertexType Type = VertexType::VertexPositionTexcoord;
	};

	struct VertexPositionNormalTexcoord
	{
		Vec4 pos;
		Vec3 normal;
		Vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[3];
		static const UINT ElementCount = 3;
		static const VertexType Type = VertexType::VertexPositionNormalTexcoord;
	};

	struct VertexPositionNormalTangentBinormalTexcoord
	{
		Vec4 pos;
		Vec3 normal;
		Vec3 tangent;
		Vec3 binormal;
		Vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[5];
		static const UINT ElementCount = 5;
		static const VertexType Type = VertexType::VertexPositionNormalTangentBinormalTexcoord;
	};

	struct VertexPositionNormalTangentBinormalTexcoordIndicesWeights
	{
		Vec4 pos;
		Vec3 normal;
		Vec3 tangent;
		Vec3 binormal;
		Vec2 texcoord;
		Index16 indices[4];
		Vec4 weights;

		static const D3D11_INPUT_ELEMENT_DESC Elements[7];
		static const UINT ElementCount = 7;
		static const VertexType Type = VertexType::VertexPositionNormalTangentBinormalTexcoordIndicesWeights;
	};
}