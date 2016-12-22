#pragma once

#include <chrono>
#include <vector>

#include "RenderSkin.h"
#include "Animation.h"

using std::string;
using std::vector;
using std::chrono::system_clock;

namespace happy
{
	using StencilMask = uint8_t;

	struct SkinRenderItem
	{
		RenderSkin m_Skin;
		float m_Alpha;
		StencilMask m_Groups;
		unsigned m_AnimationCount;

		bb::vec2 m_PreviousBlendFrame;
		bb::vec2 m_PreviousBlendAnimation;
		ID3D11Buffer* m_PreviousFrames[4];
		bb::mat4 m_PreviousWorld;

		bb::vec2 m_CurrentBlendFrame;
		bb::vec2 m_CurrentBlendAnimation;
		ID3D11Buffer* m_CurrentFrames[4];
		bb::mat4 m_CurrentWorld;

	};

	class SkinController
	{
	public:
		void setSkin(RenderSkin &skin);
		void setRenderGroups(StencilMask &groups);
		void setAlpha(float alpha);

		int addAnimation(string name, Animation animation);
		int getAnimationIndex(string name);

		void setAnimationTimer(int id, system_clock::time_point start, system_clock::duration offset);
		void setAnimationBlend(int id, float blend, system_clock::time_point start, float duration = 0);
		void setAnimationSpeed(int id, float multiplier);
		void resetAllAnimationBlends(system_clock::time_point start, float duration = 0);
		bb::mat4 &worldMatrix();

		void update(system_clock::time_point time);

		const SkinRenderItem& getRenderItem() const;

	private:
		struct anim_state
		{
			Animation m_Anim;

			string m_Name;

			system_clock::time_point m_Timer;
			system_clock::time_point m_Blender;

			float m_SpeedMultiplier;
			float m_BlendSource;
			float m_BlendTarget;
			float m_BlendDuration;

			bool m_Looped;
		};

		SkinRenderItem     m_RenderItem;
		vector<anim_state> m_States;
	};
}