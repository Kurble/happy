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

		template <>
		void operator()(const char*, bool x)
		{
			put(x);
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

			std::string type_id = n.get() ? n->get_type_id() : "nullptr_t";
			size_t node_id      = n.get() ? n->get_node_id() : (-1);

			operator()("type_id", type_id);
			operator()("node_id", node_id);

			if (n.get())
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
		BinaryDeserializer(std::istream &stream, net::node_resolver<BinaryDeserializer>* resolver = nullptr)
			: m_stream(stream)
			, m_node_resolver(resolver) { }

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

		template <>
		void operator()(const char*, bool &x)
		{
			get(x);
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
			size_t      node_id;
			std::string type_id;

			operator()("type_id", type_id);
			operator()("node_id", node_id);

			if (type_id.compare("nullptr_t"))
			{
				auto n = m_node_resolver->resolve(type_id.c_str(), node_id);
				n->reflect(*this);

				x = std::dynamic_pointer_cast<T, net::polymorphic_node>(n);
			}
			else
			{
				x = nullptr;
			}
		}

		operator bool()
		{
			return (m_stream.rdbuf()->in_avail() > 0);
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
		net::node_resolver<BinaryDeserializer>* m_node_resolver;
	};
};