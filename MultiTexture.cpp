#include "stdafx.h"
#include "MultiTexture.h"

namespace happy
{
	MultiTexture::MultiTexture()
	{
		m_ppChannels[0] = nullptr;
		m_ppChannels[1] = nullptr;
		m_ppChannels[2] = nullptr;
	}

	void MultiTexture::setChannel(unsigned channel, ComPtr<ID3D11ShaderResourceView> &texture)
	{
		m_pChannels[channel] = texture;
		m_ppChannels[channel] = texture.Get();
	}

	ID3D11ShaderResourceView** MultiTexture::getTextures() const
	{
		return (ID3D11ShaderResourceView**)m_ppChannels;
	}
}