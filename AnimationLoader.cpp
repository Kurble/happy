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

	Animation loadAnimationFromDanceFile(RenderingContext *pRenderContext, fs::path animPath)
	{
		std::ifstream fin(animPath.c_str(), std::ios::in | std::ios::binary);

		float framerate = read<float>(fin);
		uint32_t frameCount = read<uint32_t>(fin);
		uint32_t boneCount = read<uint32_t>(fin);

		vector<bb::mat4> animation;
		animation.reserve(frameCount * boneCount);

		for (unsigned f = 0; f < frameCount; ++f)
		{
			for (unsigned b = 0; b < boneCount; ++b)
			{
				bb::mat4 pose = read<bb::mat4>(fin);
				animation.push_back(pose);
			}
		}

		Animation anim;
		anim.setAnimation(pRenderContext, animation, boneCount, frameCount, framerate);
		return anim;
	}
}