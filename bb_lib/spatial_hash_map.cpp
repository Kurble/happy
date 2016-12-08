#include "spatial_hash_map.h"

namespace bb
{
	spatial_hash_map::spatial_hash_map()
		: m_Origin(0, 0)
		, m_Size(0, 0)
		, m_Resolution(0)
	{ }

	spatial_hash_map::spatial_hash_map(vec2 a, vec2 b, unsigned resolution)
	{
		resize(a, b, resolution);
	}

	void spatial_hash_map::clear()
	{
		m_ClearCounter++;
	}

	void spatial_hash_map::resize(vec2 a, vec2 b, unsigned resolution)
	{
		if (m_Origin == a &&
			m_Resolution == resolution) return;

		m_Origin = a;
		m_Size = b - a;
		m_Resolution = resolution;
		m_Grid.clear();
		m_Grid.resize(resolution * resolution);
		m_ClearCounter = 0;
	}

	void spatial_hash_map::bucket::check(unsigned clearCounter)
	{
		if (clearCounter > m_ClearCounter)
		{
			m_ClearCounter = clearCounter;
			m_Elements.clear();
		}
	}

	void spatial_hash_map::add(vec2 a, vec2 b, void *ptr)
	{
		element elem{ a, b, ptr };

		int loX = (int)(((a.x - m_Origin.x) / m_Size.x) * m_Resolution);
		int loY = (int)(((a.y - m_Origin.y) / m_Size.y) * m_Resolution);
		int hiX = (int)(((b.x - m_Origin.x) / m_Size.x) * m_Resolution);
		int hiY = (int)(((b.y - m_Origin.y) / m_Size.y) * m_Resolution);

		for (int xx = loX; xx <= hiX; ++xx)
		{
			if (xx >= 0 && xx < (int)m_Resolution)
			{
				for (int yy = loY; yy <= hiY; ++yy)
				{
					if (yy >= 0 && yy < (int)m_Resolution)
					{
						auto &bucket = m_Grid[xx + yy * m_Resolution];
						bucket.check(m_ClearCounter);
						bucket.m_Elements.push_back(elem);
					}
				}
			}
		}
	}

	void spatial_hash_map::visit(vec2 a, vec2 b, std::function<void(element)> visitor) const
	{
		int loX = (int)(((a.x - m_Origin.x) / m_Size.x) * m_Resolution);
		int loY = (int)(((a.y - m_Origin.y) / m_Size.y) * m_Resolution);
		int hiX = (int)(((b.x - m_Origin.x) / m_Size.x) * m_Resolution);
		int hiY = (int)(((b.y - m_Origin.y) / m_Size.y) * m_Resolution);

		for (int xx = loX; xx <= hiX; ++xx)
		{
			if (xx >= 0 && xx < (int)m_Resolution)
			{
				for (int yy = loY; yy <= hiY; ++yy)
				{
					if (yy >= 0 && yy < (int)m_Resolution)
					{
						auto &bucket = m_Grid[xx + yy * m_Resolution];
						if (bucket.m_ClearCounter == m_ClearCounter)
						{
							for (const auto &e : bucket.m_Elements) visitor(e);
						}
					}
				}
			}
		}
	}
}