#pragma once

#include "TextureHandle.h"

namespace happy
{
	struct MultiTexture
	{
		ComPtr<ID3D11ShaderResourceView> m_Channels[3];
	};
}