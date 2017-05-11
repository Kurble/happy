#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <map>

#include "serialize_binary.h"
#include "serialize_text.h"
#include "serialize_net.h"

namespace bb
{
	void serialize_sanity_check();

	//------------------------------------------------------------------------
	// pair serialize/deserialize
	template<typename A, typename B, typename Visitor>
	void reflect(Visitor &visit, std::pair<A, B> &x)
	{
		visit("first", x.first);
		visit("second", x.second);
	}

	//------------------------------------------------------------------------
	// polymorphic dispatch
	template<class T, typename Visitor>
	void reflect(Visitor &visitor, T& x)
	{
		x.reflect(visitor);
	}
}