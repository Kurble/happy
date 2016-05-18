#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"

namespace happy
{
	class Animation
	{
	public:
		void setAnimation(const RenderingContext *pRenderContext, const vector<Mat4> &animation, const unsigned bones, const unsigned frames, const float framerate);
		void setLooping(bool looping);

		ID3D11Buffer* getFrame0(float time) const;
		ID3D11Buffer* getFrame1(float time) const;
		float         getFrameBlend(float time) const;

	private:
		float m_FrameRate;
		bool  m_Looping;

		shared_ptr<vector<ComPtr<ID3D11Buffer>>> m_pFrames;
	};
}