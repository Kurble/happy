#include "stdafx.h"
#include "Skins.h"

namespace happy
{
	void RenderSkin::setBindPose(const RenderingContext *pRenderContext, const vector<Mat4>& pose)
	{
		D3D11_BUFFER_DESC poseDesc;
		ZeroMemory(&poseDesc, sizeof(poseDesc));

		poseDesc.ByteWidth = (UINT)pose.size() * sizeof(Mat4);
		poseDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		poseDesc.Usage = D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA poseData;
		ZeroMemory(&poseData, sizeof(poseData));
		poseData.pSysMem = (void*)&pose[0];

		THROW_ON_FAIL(pRenderContext->getDevice()->CreateBuffer(&poseDesc, &poseData, m_pBindPose.GetAddressOf()));
	}

	ID3D11Buffer* RenderSkin::getBindPoseBuffer() const
	{
		return m_pBindPose.Get();
	}

	void Animation::setAnimation(const RenderingContext *pRenderContext, const vector<Mat4> &animation, const unsigned bones, const unsigned frames, const float framerate)
	{
		m_FrameRate = framerate;
		m_Looping   = true;
		m_pFrames   = make_shared<vector<ComPtr<ID3D11Buffer>>>();

		D3D11_BUFFER_DESC poseDesc;
		ZeroMemory(&poseDesc, sizeof(poseDesc));

		poseDesc.ByteWidth = (UINT)bones * sizeof(Mat4);
		poseDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
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
	}

	void Animation::setLooping(bool looping)
	{
		m_Looping = looping;
	}
}