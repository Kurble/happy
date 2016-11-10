#pragma once
#include "VertexTypes.h"

#define FACE4(a, b, c, d) a-1, b-1, c-1, a-1, c-1, d-1
#define FACE3(a, b, c) a-1, b-1, c-1

static const happy::VertexPositionTexcoord g_CubeVertices[] = {
	{ bb::vec4( 1.0f,  1.0f, -1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4( 1.0f, -1.0f, -1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4(-1.0f, -1.0f, -1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4(-1.0f,  1.0f, -1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4( 1.0f,  1.0f,  1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4( 1.0f, -1.0f,  1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4(-1.0f, -1.0f,  1.0f,  1.0f), bb::vec2(.0f, .0f) },
	{ bb::vec4(-1.0f,  1.0f,  1.0f,  1.0f), bb::vec2(.0f, .0f) },
};


static const uint16_t g_CubeIndices[] = {
	FACE4(1, 2, 3, 4),
	FACE4(5, 8, 7, 6),
	FACE4(1, 5, 6, 2),
	FACE4(2, 6, 7, 3),
	FACE4(3, 7, 8, 4),
	FACE4(5, 1, 4, 8),
};