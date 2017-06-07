#pragma once

#include <inttypes.h>

#include "bb_lib\vec2.h"
#include "bb_lib\vec3.h"
#include "bb_lib\vec4.h"
#include "bb_lib\mat3.h"
#include "bb_lib\mat4.h"
#include "bb_lib\math_util.h"

namespace happy
{
	enum class VertexType
	{
		VertexPositionColor,
		VertexPositionTexcoord,
		VertexPositionNormalTexcoord,
		VertexPositionNormalTangentBinormalTexcoord,
		VertexPositionNormalTangentBinormalTexcoordIndicesWeights,
		Particle,
	};

	typedef uint16_t Index16;
	typedef uint32_t Index32;

	struct VertexPositionColor
	{
		bb::vec4 pos;
		bb::vec4 color;

		static const D3D11_INPUT_ELEMENT_DESC Elements[2];
		static const UINT ElementCount = 2;
		static const VertexType Type = VertexType::VertexPositionColor;
	};

	struct VertexPositionTexcoord
	{
		bb::vec4 pos;
		bb::vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[2];
		static const UINT ElementCount = 2;
		static const VertexType Type = VertexType::VertexPositionTexcoord;
	};

	struct VertexPositionNormalTexcoord
	{
		bb::vec4 pos;
		bb::vec3 normal;
		bb::vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[3];
		static const UINT ElementCount = 3;
		static const VertexType Type = VertexType::VertexPositionNormalTexcoord;
	};

	struct VertexPositionNormalTangentBinormalTexcoord
	{
		bb::vec4 pos;
		bb::vec3 normal;
		bb::vec3 tangent;
		bb::vec3 binormal;
		bb::vec2 texcoord;

		static const D3D11_INPUT_ELEMENT_DESC Elements[5];
		static const UINT ElementCount = 5;
		static const VertexType Type = VertexType::VertexPositionNormalTangentBinormalTexcoord;
	};

	struct VertexPositionNormalTangentBinormalTexcoordIndicesWeights
	{
		bb::vec4 pos;
		bb::vec3 normal;
		bb::vec3 tangent;
		bb::vec3 binormal;
		bb::vec2 texcoord;
		Index16 indices[4];
		bb::vec4 weights;

		static const D3D11_INPUT_ELEMENT_DESC Elements[7];
		static const UINT ElementCount = 7;
		static const VertexType Type = VertexType::VertexPositionNormalTangentBinormalTexcoordIndicesWeights;
	};

	struct VertexParticle
	{
		bb::vec4 position;
		bb::vec4 lifeSizeGrowWiggle;
		bb::vec4 velocityFriction;
		bb::vec4 color;

		static const D3D11_INPUT_ELEMENT_DESC Elements[4];
		static const UINT ElementCount = 4;
		static const VertexType Type = VertexType::Particle;
	};
}