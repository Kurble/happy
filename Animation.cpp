#include "stdafx.h"
#include "Animation.h"

namespace happy
{
	void Animation::setAnimation(const RenderingContext *pRenderContext, const vector<bb::mat4> &animation, const unsigned bones, const unsigned frames, const float framerate)
	{
		m_FrameRate = framerate;
		m_Looping = true;
		m_pFrames = make_shared<vector<ComPtr<ID3D11Buffer>>>();

		D3D11_BUFFER_DESC poseDesc;
		ZeroMemory(&poseDesc, sizeof(poseDesc));

		poseDesc.ByteWidth = (UINT)bones * sizeof(bb::mat4);
		poseDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		poseDesc.Usage = D3D11_USAGE_IMMUTABLE;

		auto &bufs = *m_pFrames.get();
		bufs.reserve(frames);
		for (unsigned i = 0; i < frames; ++i)
		{
			D3D11_SUBRESOURCE_DATA poseData;
			ZeroMemory(&poseData, sizeof(poseData));
			poseData.pSysMem = (void*)&animation[i * bones];

			ComPtr<ID3D11Buffer> pose;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&poseDesc, &poseData, pose.GetAddressOf()));

			bufs.push_back(pose);
		}

		if (frames == 0)
		{
			bb::mat4 identity;
			identity.identity();
			vector<bb::mat4> stillFrame;
			stillFrame.resize(bones, identity);

			D3D11_SUBRESOURCE_DATA poseData;
			ZeroMemory(&poseData, sizeof(poseData));
			poseData.pSysMem = (void*)stillFrame.data();

			ComPtr<ID3D11Buffer> pose;
			THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&poseDesc, &poseData, pose.GetAddressOf()));

			bufs.push_back(pose);
		}
	}

	void Animation::setLooping(bool looping)
	{
		m_Looping = looping;
	}

	ID3D11Buffer* Animation::getFrame0(float time) const
	{
		unsigned frame = (unsigned)floorf(time * m_FrameRate);
		if (!m_Looping) frame = min((unsigned)m_pFrames->size() - 1, frame);
		else            frame = (frame % m_pFrames->size());

		return m_pFrames->at(frame).Get();
	}

	ID3D11Buffer* Animation::getFrame1(float time) const
	{
		unsigned frame = (unsigned)floorf(time * m_FrameRate) + 1;
		if (!m_Looping) frame = min((unsigned)m_pFrames->size() - 1, frame);
		else            frame = (frame % m_pFrames->size());

		return m_pFrames->at(frame).Get();
	}

	float Animation::getFrameBlend(float time) const
	{
		return (time * m_FrameRate) - floorf(time * m_FrameRate);
	}
}