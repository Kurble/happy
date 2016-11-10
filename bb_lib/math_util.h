#pragma once

namespace bb
{
	static const float epsilon = 0.00001f;

	template <class T> T lerp(T a, T b, float x)
	{
		return a*(1 - x) + b*x;
	}
}
