#include "SkinController.h"
#include "stdafx.h"
#include "SkinController.h"

#include <algorithm>

namespace happy
{
	static float resolveBlend(system_clock::time_point start, system_clock::time_point current, float source, float target, float duration)
	{
		if (duration)
		{
			std::chrono::duration<float, std::ratio<1, 1>> offset(current - start);
			return lerp(source, target, fminf(1.0f, offset.count() / duration));
		}
		return target;
	}

	void SkinController::setSkin(RenderSkin &skin)
	{
		m_RenderItem.m_Skin = skin;

		m_RenderItem.m_AnimationCount = 1;
		m_RenderItem.m_World.identity();
		m_RenderItem.m_World.scale(Vec3(1, 1, 1) * 0.5f);
		m_RenderItem.m_BlendAnimation = Vec4(1, 0, 0, 0);
		m_RenderItem.m_BlendFrame = Vec4(0, 0, 0, 0);
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

	void SkinController::resetAllAnimationBlends()
	{
		for (auto &s : m_States)
		{
			s.m_BlendDuration = 0;
			s.m_BlendSource = 0;
			s.m_BlendTarget = 0;
		}
	}

	Mat4 &SkinController::worldMatrix()
	{
		return m_RenderItem.m_World;
	}

	void SkinController::update(system_clock::time_point time)
	{
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

		m_RenderItem.m_AnimationCount = min(4, (unsigned)influences.size());
		float total = 0;
		for (unsigned a = 0; a < m_RenderItem.m_AnimationCount; ++a)
		{
			auto &state = m_States[influences[a].first];
			std::chrono::duration<float, std::ratio<1, 1>> timer(time - state.m_Timer);

			float x = timer.count() * state.m_SpeedMultiplier;
			m_RenderItem.m_Frames[a * 2 + 0] = state.m_Anim.getFrame0(x);
			m_RenderItem.m_Frames[a * 2 + 1] = state.m_Anim.getFrame1(x);
			m_RenderItem.m_BlendFrame[a] = state.m_Anim.getFrameBlend(x);

			m_RenderItem.m_BlendAnimation[a] = influences[a].second;
			total += influences[a].second;
		}

		m_RenderItem.m_BlendAnimation = m_RenderItem.m_BlendAnimation * (1.0f / total);
	}

	const SkinRenderItem& SkinController::getRenderItem() const
	{		
		return m_RenderItem;
	}
}