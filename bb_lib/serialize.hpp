#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <type_traits>

namespace bb
{
	namespace net
	{
		class polymorphic_node;

		template <typename Deserializer>
		class node_factory_base
		{
		public:
			virtual ~node_factory_base() { }

			virtual std::shared_ptr<polymorphic_node> make_node(Deserializer& deserializer) = 0;
		};
	}
}

#include "serialize_binary.hpp"
#include "serialize_text.hpp"
#include "serialize_partial.hpp"

namespace bb
{
	void serialize_sanity_check();

	//----------------------------------------------------------------------------------------------------------------
	// pair serialize/deserialize
	template<typename A, typename B, typename Visitor>
	void reflect(Visitor &visit, std::pair<A, B> &x)
	{
		visit("first", x.first);
		visit("second", x.second);
	}

	//----------------------------------------------------------------------------------------------------------------
	// polymorphic dispatch
	template<class T, typename Visitor>
	void reflect(Visitor &visitor, T& x)
	{
		x.reflect(visitor);
	}
}