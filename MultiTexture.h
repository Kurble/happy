#pragma once

#include "TextureHandle.h"

namespace happy
{
	class MultiTexture
	{
	public:
		MultiTexture();

		void setChannel(unsigned channel, ComPtr<ID3D11ShaderResourceView> &texture);

		ID3D11ShaderResourceView** getTextures() const;

	private:
		ComPtr<ID3D11ShaderResourceView> m_pChannels[3];
		ID3D11ShaderResourceView *m_ppChannels[3];
	};
}