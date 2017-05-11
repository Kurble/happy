#pragma once

#include "serialize.hpp"

#define DECLARE_NET_NODE_REFLECT() \
	virtual void dispatchReflect(BinarySerializer& s); \
	virtual void dispatchReflect(BinaryDeserializer& s); \
	virtual void dispatchReflect(TextSerializer& s); \
	virtual void dispatchReflect(TextDeserializer& s); \
	template <typename VISITOR> void reflect(VISITOR& visitor);

namespace bb
{
	namespace net
	{
		class context;

		class node
		{
		public:
			node(context *context);

			DECLARE_NET_NODE_REFLECT();

			context* get_context() { return m_context; }

		private:
			context *m_context;
		};
	}
}