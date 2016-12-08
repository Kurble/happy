#pragma once

#include <functional>
#include <vector>

#include "vec2.h"

namespace bb
{
	class spatial_hash_map
	{
	public:
		spatial_hash_map();
		spatial_hash_map(vec2 a, vec2 b, unsigned resolution);

		struct element
		{
			vec2 m_A;
			vec2 m_B;
			void* m_Ptr;
		};

		void clear();
		void resize(vec2 a, vec2 b, unsigned resolution);
		void add(vec2 a, vec2 b, void* user);
		void visit(vec2 a, vec2 b, std::function<void(element)> visitor) const;

	private:
		vec2 m_Origin;
		vec2 m_Size;
		unsigned m_Resolution;

		unsigned m_ClearCounter = 0;

		struct bucket
		{
			void check(unsigned clearCounter);

			std::vector<element> m_Elements;
			unsigned m_ClearCounter = 0;
		};

		std::vector<bucket> m_Grid;
	};
}