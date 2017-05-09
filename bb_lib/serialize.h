#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <map>

namespace bb
{
	void serialize_sanity_check();

	//------------------------------------------------------------------------
	// Tag system
	struct Tag
	{
		const std::string m_Name;
		const uint32_t    m_ID;

		static Tag make(const char* name);

	private:
		Tag(std::string name, uint32_t id)
			: m_Name(name)
			, m_ID(id) {}
	};

	//------------------------------------------------------------------------
	// Generic binary serializer
	class BinarySerializer
	{
	public:
		BinarySerializer(std::ostream &stream) :m_stream(stream) { }

		template <typename T> void put(const T& val)
		{
			m_stream.write(reinterpret_cast<const char*>(&val), sizeof(T));
		}

		void put(const char* data, size_t size)
		{
			m_stream.write(data, size);
		}

	private:
		std::ostream &m_stream;
	};

	//------------------------------------------------------------------------
	// Generic binary deserializer
	class BinaryDeserializer
	{
	public:
		BinaryDeserializer(std::istream &stream) :m_stream(stream) { }

		template <typename T> void get(T& val)
		{
			m_stream.read(reinterpret_cast<char*>(&val), sizeof(T));
		}

		void get(char* data, size_t size)
		{
			m_stream.read(data, size);
		}

	private:
		std::istream &m_stream;
	};

	//------------------------------------------------------------------------
	// Fundamental serializers
	#define INSTANTIATE_SERIALIZE_FUNDAMENTAL(TYPE)\
	inline void serialize(TYPE value, BinarySerializer &s)\
	{\
		s.put(value); \
	}\
	inline void serialize(TYPE& value, BinaryDeserializer &s)\
	{\
		s.get(value); \
	}

	INSTANTIATE_SERIALIZE_FUNDAMENTAL(int8_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(uint8_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(int16_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(uint16_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(int32_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(uint32_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(int64_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(uint64_t)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(float)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(double)
	INSTANTIATE_SERIALIZE_FUNDAMENTAL(bool)

	#undef DECLARE_SERIALIZE_FUNDAMENTAL

	void serialize(std::string& value, BinarySerializer &s);
	void serialize(std::string& value, BinaryDeserializer &s);

	//------------------------------------------------------------------------
	// vector serialize
	template<typename T>
	void serialize(std::vector<T> &x, BinarySerializer &s)
	{
		size_t size = x.size();
		serialize(size, s);
		for (size_t i = 0; i < x.size(); ++i)
		{
			serialize(x[i], s);
		}
	}
	template<typename T>
	void serialize(std::vector<T> &x, BinaryDeserializer &s)
	{
		size_t size;
		serialize(size, s);

		x.clear();
		x.resize(size);

		for (size_t i = 0; i < size; ++i)
		{
			serialize(x[i], s);
		}
	}

	//------------------------------------------------------------------------
	// pair serialize
	template<typename A, typename B>
	void serialize(std::pair<A, B> &x, BinarySerializer &s)
	{
		serialize(x.first, s);
		serialize(x.second, s);
	}
	template<typename A, typename B>
	void serialize(std::pair<A, B> &x, BinaryDeserializer &s)
	{
		serialize(x.first, s);
		serialize(x.second, s);
	}

	//------------------------------------------------------------------------
	// map serialize
	template<typename K, typename V>
	void serialize(std::map<K, V> &x, BinarySerializer &s)
	{
		size_t size = x.size();
		serialize(size, s);
		for (auto &kv : x)
		{
			serialize(kv, s);
		}
	}
	template<typename K, typename V>
	void serialize(std::map<K, V> &x, BinaryDeserializer &s)
	{
		size_t size;
		serialize(size, s);

		x.clear();

		for (size_t i = 0; i < size; ++i)
		{
			std::pair<K, V> kv;
			serialize(kv, s);
			x.insert(kv);
		}
	}
};