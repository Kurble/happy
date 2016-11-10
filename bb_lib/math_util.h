#pragma once

namespace bb
{
	static const float epsilon = 0.00001f;

	template <class T> T lerp(T a, T b, float x)
	{
		return a*(1 - x) + b*x;
	}

	template <typename T> T swap_endian(T u)
	{
		union
		{
			T u;
			unsigned char u8[sizeof(T)];
		} source, dest;

		source.u = u;
		
		for (size_t k = 0; k < sizeof(T); k++)
			dest.u8[k] = source.u8[sizeof(T) - k - 1];

		return dest.u;
	}
}
