#pragma once

#include "serialize.hpp"

#include <memory>

#define DECLARE_NET_NODE_REFLECT() \
	virtual void polymorphicReflect(BinarySerializer& visitor); \
	virtual void polymorphicReflect(BinaryDeserializer& visitor); \
	virtual void polymorphicReflect(TextSerializer& visitor); \
	virtual void polymorphicReflect(TextDeserializer& visitor); \
	virtual void polymorphicReflect(partial::UpdateVisitor<BinarySerializer>& visitor); \
	virtual void polymorphicReflect(partial::UpdateVisitor<BinaryDeserializer>& visitor); \
	virtual void polymorphicReflect(partial::UpdateVisitor<TextSerializer>& visitor); \
	virtual void polymorphicReflect(partial::UpdateVisitor<TextDeserializer>& visitor); \
	template <typename VISITOR> void reflect(VISITOR& visitor);

#define IMPLEMENT_NET_NODE_REFLECT(TypeName) \
	void TypeName::polymorphicReflect(BinarySerializer& visitor)                           { reflect(visitor); } \
	void TypeName::polymorphicReflect(BinaryDeserializer& visitor)                         { reflect(visitor); } \
	void TypeName::polymorphicReflect(TextSerializer& visitor)                             { reflect(visitor); } \
	void TypeName::polymorphicReflect(TextDeserializer& visitor)                           { reflect(visitor); } \
	void TypeName::polymorphicReflect(partial::UpdateVisitor<BinarySerializer>& visitor)   { reflect(visitor); } \
	void TypeName::polymorphicReflect(partial::UpdateVisitor<BinaryDeserializer>& visitor) { reflect(visitor); } \
	void TypeName::polymorphicReflect(partial::UpdateVisitor<TextSerializer>& visitor)     { reflect(visitor); } \
	void TypeName::polymorphicReflect(partial::UpdateVisitor<TextDeserializer>& visitor)   { reflect(visitor); } \
	template <typename VISITOR> void TypeName::reflect(VISITOR& visitor)

namespace bb
{
	namespace net
	{
		class context;

		typedef size_t node_id;

		class node
		{
		public:
			node(node_id id, context* context);
			virtual ~node();

			DECLARE_NET_NODE_REFLECT();

			context* get_context() { return m_context; }

			node_id get_node_id() { return m_id; }

		private:
			context* m_context;
			node_id  m_id;
		};

		template<typename T, typename Visitor>
		typename std::enable_if<std::is_base_of<node, T>::value, void>::type reflect(Visitor &visitor, std::shared_ptr<T>& x)
		{
			x->dispatchReflect(visitor);
		}
	}
}