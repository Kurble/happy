#pragma once

namespace happy
{
	struct CBufferScene
	{
		bb::mat4 view;
		bb::mat4 projection;
		bb::mat4 viewInverse;
		bb::mat4 projectionInverse;
		float width;
		float height;
		unsigned int convolutionStages;
		unsigned int aoEnabled;
	};

	struct CBufferTAA
	{
		bb::mat4 viewInverse;
		bb::mat4 projectionInverse;
		bb::mat4 viewHistory;
		bb::mat4 projectionHistory;
		float blendFactor;
		float texelWidth;
		float texelHeight;
	};

	struct CBufferObject
	{
		bb::mat4 world;
		bb::mat4 worldInverse;
		float alpha;
	};

	struct CBufferSkin
	{
		float blendAnim[4];
		float blendFrame[4];
		unsigned int animationCount;
	};

	struct CBufferPointLight
	{
		bb::vec4 position;
		bb::vec4 color;
		float scale;
		float falloff;
	};

	struct CBufferSSAO
	{
		float occlusionRadius = 0.1f;
		int   samples = 128;
		float invSamples = 1.0f / 128.0f;

		bb::vec3 random[512];
	};
}