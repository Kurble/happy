#include "SkinController.h"
#include "stdafx.h"
#include "SkinController.h"

#include <algorithm>

namespace happy
{
	static float resolveBlend(system_clock::time_point start, system_clock::time_point current, float source, float target, float duration)
	{
		if (duration && source != target)
		{
			std::chrono::duration<float, std::ratio<1, 1>> offset(current - start);
			return bb::lerp(source, target, fminf(1.0f, offset.count() / duration));
		}
		return target;
	}

	void SkinController::setSkin(RenderSkin &skin)
	{
		m_RenderItem.m_Skin = skin;

		m_RenderItem.m_Alpha = 1.0f;
		m_RenderItem.m_AnimationCount = 1;
		m_RenderItem.m_Groups = 0x00;
		m_RenderItem.m_CurrentBlendAnimation = bb::vec2(1, 0);
		m_RenderItem.m_CurrentBlendFrame = bb::vec2(0, 0);
		memset(m_RenderItem.m_CurrentFrames, 0, sizeof(m_RenderItem.m_CurrentFrames));
		m_RenderItem.m_CurrentWorld.identity();
		m_RenderItem.m_CurrentWorld.scale(bb::vec3(1, 1, 1) * 0.5f);
		m_RenderItem.m_PreviousBlendAnimation = m_RenderItem.m_CurrentBlendAnimation;
		m_RenderItem.m_PreviousBlendFrame = m_RenderItem.m_CurrentBlendFrame;
		memcpy(m_RenderItem.m_PreviousFrames, m_RenderItem.m_CurrentFrames, sizeof(m_RenderItem.m_CurrentFrames));
		m_RenderItem.m_PreviousWorld = m_RenderItem.m_CurrentWorld;
	}

	void SkinController::setAlpha(float alpha)
	{
		m_RenderItem.m_Alpha = alpha;
	}

	void SkinController::setRenderGroups(StencilMask &groups)
	{
		m_RenderItem.m_Groups = groups;
	}

	int SkinController::addAnimation(string name, Animation animation)
	{
		anim_state state;

		state.m_Name = name;
		state.m_Anim = animation;
		state.m_Blender = system_clock::now();
		state.m_Timer = system_clock::now();
		state.m_SpeedMultiplier = 1;
		state.m_BlendDuration = 0;
		state.m_BlendSource = 0;
		state.m_BlendTarget = 0;
		state.m_Looped = false;

		m_States.push_back(state);
		return (int)(m_States.size() - 1);
	}

	int SkinController::getAnimationIndex(string name)
	{
		for (unsigned index = 0; index < m_States.size(); ++index)
		{
			if (name == m_States[index].m_Name) return index;
		}
		return -1;
	}

	void SkinController::setAnimationTimer(int id, system_clock::time_point start, system_clock::duration offset)
	{
		m_States[id].m_Timer = start - offset;
	}

	void SkinController::setAnimationBlend(int id, float blend, system_clock::time_point start, float duration)
	{
		auto &s = m_States[id];

		s.m_BlendSource = resolveBlend(s.m_Blender, start, s.m_BlendSource, s.m_BlendTarget, s.m_BlendDuration);
		s.m_BlendTarget = blend;
		s.m_Blender = start;
		s.m_BlendDuration = duration;
	}

	void SkinController::setAnimationSpeed(int id, float multiplier)
	{
		m_States[id].m_SpeedMultiplier = multiplier;
	}

	void SkinController::resetAllAnimationBlends(system_clock::time_point start, float duration)
	{
		for (auto &s : m_States)
		{
			s.m_BlendSource = resolveBlend(s.m_Blender, start, s.m_BlendSource, s.m_BlendTarget, s.m_BlendDuration);
			s.m_BlendTarget = 0;
			s.m_Blender = start;
			s.m_BlendDuration = duration;
		}
	}

	bb::mat4 &SkinController::worldMatrix()
	{
		return m_RenderItem.m_CurrentWorld;
	}

	void SkinController::update(system_clock::time_point time)
	{
		m_RenderItem.m_PreviousBlendAnimation = m_RenderItem.m_CurrentBlendAnimation;
		m_RenderItem.m_PreviousBlendFrame = m_RenderItem.m_CurrentBlendFrame;
		memcpy(m_RenderItem.m_PreviousFrames, m_RenderItem.m_CurrentFrames, sizeof(m_RenderItem.m_CurrentFrames));
		m_RenderItem.m_PreviousWorld = m_RenderItem.m_CurrentWorld;

		vector<pair<unsigned,float>> influences;
		for (unsigned i = 0; i < m_States.size(); ++i)
		{
			float blend = resolveBlend(m_States[i].m_Blender, time, m_States[i].m_BlendSource, m_States[i].m_BlendTarget, m_States[i].m_BlendDuration);

			if (blend) influences.emplace_back(i, blend);
		}

		std::sort(influences.begin(), influences.end(), [](const pair<unsigned, float>& a, const pair<unsigned, float>& b) 
		{
			return a.second > b.second;
		});

		m_RenderItem.m_AnimationCount = min(2, (unsigned)influences.size());
		float total = 0;
		for (unsigned a = 0; a < m_RenderItem.m_AnimationCount; ++a)
		{
			auto &state = m_States[influences[a].first];
			std::chrono::duration<float, std::ratio<1, 1>> timer(time - state.m_Timer);

			float x = timer.count() * state.m_SpeedMultiplier;
			m_RenderItem.m_CurrentFrames[a * 2 + 0] = state.m_Anim.getFrame0(x);
			m_RenderItem.m_CurrentFrames[a * 2 + 1] = state.m_Anim.getFrame1(x);
			m_RenderItem.m_CurrentBlendFrame[a] = state.m_Anim.getFrameBlend(x);

			m_RenderItem.m_CurrentBlendAnimation[a] = influences[a].second;
			total += influences[a].second;
		}

		m_RenderItem.m_CurrentBlendAnimation = m_RenderItem.m_CurrentBlendAnimation * (1.0f / total);
	}

	const SkinRenderItem& SkinController::getRenderItem() const
	{		
		return m_RenderItem;
	}
}