#pragma once

namespace bb
{
	//------------------------------------------------------------------------
	// Generic binary serializer
	class BinarySerializer
	{
	public:
		BinarySerializer(std::ostream &stream) 
			: m_stream(stream) { }

		// arithmetic types
		template <typename T>
		typename std::enable_if<std::is_arithmetic<T>::value, void>::type operator()(const char*, T x)
		{
			put(x);
		}

		// enums
		template <typename T>
		typename std::enable_if<std::is_enum<T>::value, void>::type operator()(const char*, T x)
		{
			put((size_t)x);
		}

		// dispatch serialization to reflect API
		template <class T>
		typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type operator()(const char*, T &x)
		{
			reflect(*this, x);
		}

		// string serialization
		template <>
		void operator()(const char*, std::string &x)
		{
			put(x.size());
			put(x.data(), x.size());
		}

		// vector serialization
		template <typename T>
		void operator()(const char*, std::vector<T> &x)
		{
			put(x.size());
			for (auto &elem : x)
				operator()(nullptr, elem);
		}

		// map serialization
		template <typename K, typename V>
		void operator()(const char*, std::map<K, V> &x)
		{
			put(x.size());
			for (auto &elem : x)
				reflect(*this, elem);
		}

		// net::node serialization
		template <typename T>
		void operator()(const char*, std::shared_ptr<T> &x)
		{
			std::shared_ptr<net::polymorphic_node> n = std::dynamic_pointer_cast<net::polymorphic_node, T>(x);
			n->reflect(*this);
		}

		operator bool()
		{
			return true;
		}

	private:
		template <typename T> void put(const T& val)
		{
			m_stream.write(reinterpret_cast<const char*>(&val), sizeof(T));
		}

		void put(const char* data, size_t size)
		{
			m_stream.write(data, size);
		}

		std::ostream &m_stream;
	};
	
	//------------------------------------------------------------------------
	// Generic binary deserializer
	class BinaryDeserializer
	{
	public:
		BinaryDeserializer(std::istream &stream, net::node_factory_base<BinaryDeserializer>* factory = nullptr)
			: m_stream(stream)
			, m_node_factory(factory) { }

		// arithmetic types
		template <typename T>
		typename std::enable_if< std::is_arithmetic<T>::value, void>::type operator()(const char*, T &x)
		{
			get(x);
		}

		// enums
		template <typename T>
		typename std::enable_if< std::is_enum<T>::value, void>::type operator()(const char*, T &x)
		{
			size_t _x;
			get(_x);
			x = (T)_x;
		}

		// dispatch deserialization to reflect API
		template <class T>
		typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_enum<T>::value, void>::type operator()(const char*, T &x)
		{
			reflect(*this, x);
		}

		// string deserialization
		template <>
		void operator()(const char*, std::string &x)
		{
			size_t size;
			get(size);
			x.resize(size);
			get(&x[0], size);
		}

		// vector deserialization
		template <typename T>
		void operator()(const char*, std::vector<T> &x)
		{
			size_t size;
			get(size);
			
			x.clear();
			x.reserve(size);
			for (size_t i = 0; i < size; ++i)
			{
				T elem; operator()(nullptr, elem);
				x.push_back(elem);
			}
		}

		// map deserialization
		template <typename K, typename V>
		void operator()(const char*, std::map<K, V> &x)
		{
			size_t size;
			get(size);

			x.clear();
			for (size_t i = 0; i < size; ++i)
			{
				std::pair<K, V> elem; reflect(*this, elem);
				x.insert(elem);
			}
		}

		// net::node deserialization
		template <typename T>
		void operator()(const char* tag, std::shared_ptr<T> &x)
		{
			if (x.get() == nullptr)
			{
				x = std::dynamic_pointer_cast<T, net::polymorphic_node>(m_node_factory->make_node(*this));
			}
			else
			{
				std::shared_ptr<net::polymorphic_node> n = std::dynamic_pointer_cast<net::polymorphic_node, T>(x);
				n->reflect(*this);
			}
		}

		operator bool()
		{
			return m_stream.rdbuf()->in_avail() > 0;
		}

	private:
		template <typename T> void get(T& val)
		{
			m_stream.read(reinterpret_cast<char*>(&val), sizeof(T));
		}

		void get(char* data, size_t size)
		{
			m_stream.read(data, size);
		}

		std::istream &m_stream;
		net::node_factory_base<BinaryDeserializer>* m_node_factory;
	};
};