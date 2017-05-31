#pragma once

namespace happy
{
	struct SurfaceShader
	{
		SurfaceShader() {}

		virtual ~SurfaceShader() {}

		bool operator<(const SurfaceShader& o) const
		{
			return m_Handle.Get() < o.m_Handle.Get();
		}

	protected:
		friend class Resources;
		friend class DeferredRenderer;
		friend class Canvas;

		ComPtr<ID3D11PixelShader> m_Handle;
	};
}
