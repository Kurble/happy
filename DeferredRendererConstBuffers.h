#pragma once

namespace happy
{
	template <typename T>
	void updateConstantBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, const T &value)
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		THROW_ON_FAIL(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, (void*)&value, ((sizeof(T) + 15) / 16) * 16);
		context->Unmap(buffer, 0);
	}

	struct CBufferScene
	{
		bb::mat4 jitteredView;
		bb::mat4 jitteredProjection;
		bb::mat4 inverseView;
		bb::mat4 inverseProjection;
		bb::mat4 currentView;
		bb::mat4 currentProjection;
		bb::mat4 previousView;
		bb::mat4 previousProjection;
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
		bb::mat4 currentWorld;
		bb::mat4 inverseWorld;
		bb::mat4 previousWorld;
		float alpha;
	};

	struct CBufferSkin
	{
		float previousBlendAnim[2];
		float previousBlendFrame[2];
		float currentBlendAnim[2];
		float currentBlendFrame[2];
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