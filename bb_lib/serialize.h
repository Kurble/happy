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
	// Generic binary serializer
	class BinarySerializer
	{
	public:
		BinarySerializer(std::ostream &stream) :m_stream(stream) { }

		template <class T>
		void operator()(const char*, T& x)
		{
			serialize(x, *this);
		}

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
	// Generic text serializer
	class TextSerializer
	{
	public:
		TextSerializer(std::ostream &stream) 
			: m_stream(stream)
			, m_indentation(0) { }

		template <class T>
		void operator()(const char* tag, T& x)
		{
			line() << tag << " {";
			indent();
			serialize(x, *this);
			unindent();
			line() << "}";
		}

		template<typename T>
		void operator()(const char* tag, std::vector<T>& x)
		{
			line() << tag << " ";
			serialize(x, *this);
		}

		template<typename K, typename V>
		void operator()(const char* tag, std::map<K,V>& x)
		{
			line() << tag << " ";
			serialize(x, *this);
		}

		#define VISIT_FUNDAMENTAL(TYPE)\
		template<> \
		void operator()(const char* tag, TYPE& value)\
		{\
			line() << tag << " " << value;\
		}\
		template<> \
		void operator()(const char* tag, const TYPE& value)\
		{\
			line() << tag << " " << value;\
		}
		VISIT_FUNDAMENTAL(int8_t)
		VISIT_FUNDAMENTAL(uint8_t)
		VISIT_FUNDAMENTAL(int16_t)
		VISIT_FUNDAMENTAL(uint16_t)
		VISIT_FUNDAMENTAL(int32_t)
		VISIT_FUNDAMENTAL(uint32_t)
		VISIT_FUNDAMENTAL(int64_t)
		VISIT_FUNDAMENTAL(uint64_t)
		VISIT_FUNDAMENTAL(float)
		VISIT_FUNDAMENTAL(double)
		VISIT_FUNDAMENTAL(std::string)
		#undef VISIT_FUNDAMENTAL

		std::ostream& stream() 
		{ 
			return m_stream; 
		}

		std::ostream& line()
		{
			m_stream << std::endl;
			for (int i = 0; i < m_indentation; ++i)
				m_stream << "    ";
			return m_stream;
		}

		void indent() { m_indentation++; }
		void unindent() { m_indentation--; }

	private:
		std::ostream &m_stream;
		int m_indentation;
	};

	//------------------------------------------------------------------------
	// Generic binary deserializer
	class BinaryDeserializer
	{
	public:
		BinaryDeserializer(std::istream &stream) :m_stream(stream) { }

		template <class T>
		void operator()(const char*, T& x)
		{
			serialize(x, *this);
		}

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
	#define SERIALIZE_FUNDAMENTAL(TYPE)\
		inline void serialize(TYPE  value, BinarySerializer &s)\
		{ \
			s.put(value); \
		} \
		inline void serialize(TYPE& value, BinaryDeserializer &s)\
		{ \
			s.get(value); \
		}
	SERIALIZE_FUNDAMENTAL(int8_t)
	SERIALIZE_FUNDAMENTAL(uint8_t)
	SERIALIZE_FUNDAMENTAL(int16_t)
	SERIALIZE_FUNDAMENTAL(uint16_t)
	SERIALIZE_FUNDAMENTAL(int32_t)
	SERIALIZE_FUNDAMENTAL(uint32_t)
	SERIALIZE_FUNDAMENTAL(int64_t)
	SERIALIZE_FUNDAMENTAL(uint64_t)
	SERIALIZE_FUNDAMENTAL(float)
	SERIALIZE_FUNDAMENTAL(double)
	#undef SERIALIZE_FUNDAMENTAL

	//------------------------------------------------------------------------
	// string serialize
	inline 
	void serialize(std::string& value, BinarySerializer &s)
	{
		size_t size = value.size();
		serialize(size, s);
		s.put(value.data(), value.size());
	}
	inline
	void serialize(std::string& value, TextSerializer &s)
	{
		s.stream() << "\"" << value << "\"";
	}
	//------------------------------------------------------------------------
	// string deserialize
	inline 
	void serialize(std::string& value, BinaryDeserializer &s)
	{
		size_t size;
		serialize(size, s);
		value.resize(size);
		s.get(&value[0], value.size());
	}
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
	void serialize(std::vector<T> &x, TextSerializer &s)
	{
		s.stream() << "[";
		s.indent();
		for (auto &elem : x)
		{
			s.line();
			serialize(elem, s);
		}
		s.unindent();
		s.line() << "]";
	}
	//------------------------------------------------------------------------
	// vector deserialize
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
	// pair serialize/deserialize
	template<typename A, typename B, typename Visitor>
	void serialize(std::pair<A, B> &x, Visitor &visit)
	{
		visit("first", x.first);
		visit("second", x.second);
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
	void serialize(std::map<K, V> &x, TextSerializer &s)
	{
		s.stream() << "[";
		s.indent();
		for (auto &elem : x)
		{
			s.line() << "{";
			s.indent();
			serialize(elem, s);
			s.unindent();
			s.line() << "}";

		}
		s.unindent();
		s.line() << "]";
	}
	//------------------------------------------------------------------------
	// map deserialize
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