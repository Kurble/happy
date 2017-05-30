#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <type_traits>
#include <filesystem>

namespace bb
{
	namespace net
	{
		class polymorphic_node;

		template <typename Deserializer>
		class node_resolver
		{
		public:
			virtual ~node_resolver() { }

			virtual std::shared_ptr<polymorphic_node> resolve(const char* tid, size_t nid) = 0;
		};
	}
}

#include "serialize_binary.hpp"
#include "serialize_text.hpp"

namespace bb
{
	namespace fs = std::experimental::filesystem;

	void serialize_sanity_check();

	//----------------------------------------------------------------------------------------------------------------
	// pair serialize/deserialize
	template<typename A, typename B, typename Visitor>
	void reflect(Visitor &visit, std::pair<A, B> &x)
	{
		visit("key", x.first);
		visit("val", x.second);
	}

	//----------------------------------------------------------------------------------------------------------------
	// fs::path serialize/deserialize
	template <typename Visitor>
	void reflect(Visitor& visit, fs::path& x)
	{
		std::string str = x.string();

		visit("path", str);

		x = str;
	}

	//----------------------------------------------------------------------------------------------------------------
	// polymorphic dispatch
	template<class T, typename Visitor>
	void reflect(Visitor &visitor, T& x)
	{
		x.reflect(visitor);
	}
}