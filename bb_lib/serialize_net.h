#pragma once

#include "serialize.h"

namespace bb
{
	//------------------------------------------------------------------------
	// Update serializer (for net)
	class UpdateSerializer
	{
	public:
		UpdateSerializer(const char *tag, std::ostream &stream)
			: m_tag(tag)
			, m_ser(stream)
		{
			m_ser("tag", m_tag);
		}

		template <class T>
		void operator()(const char* tag, T& x)
		{
			if (m_tag.compare(tag) == 0)
				m_ser(tag, x);
		}

	private:
		std::string      m_tag;
		BinarySerializer m_ser;
	};

	//------------------------------------------------------------------------
	// Update deserializer (for net)
	class UpdateDeserializer
	{
	public:
		UpdateDeserializer(std::istream &stream)
			: m_ser(stream)
		{
			m_ser("tag", m_tag);
		}

		template <class T>
		void operator()(const char* tag, T& x)
		{
			if (m_tag.compare(tag) == 0)
				serialize(x, m_ser);
		}

	private:
		std::string        m_tag;
		BinaryDeserializer m_ser;
	};

	template <typename T> void tag_modified(T& object, const char* tag, std::ostream &o)
	{
		serialize(object, UpdateSerializer(tag, o));
	}
}