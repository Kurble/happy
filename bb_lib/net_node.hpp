#pragma once

#include "serialize.hpp"

#include <memory>

namespace bb
{
	namespace net
	{
		class context;

		typedef size_t node_id;

		typedef const char* type_id;

		class polymorphic_node : public std::enable_shared_from_this<polymorphic_node>
		{
		public:
			virtual ~polymorphic_node() { }

			template <class T> 
			std::shared_ptr<T> cast()
			{
				return std::dynamic_pointer_cast<T, polymorphic_node>(shared_from_this());
			}

			template <typename VISITOR>
			void reflect(VISITOR& visitor)
			{
				this->pm_reflect(visitor);
			}

			virtual void     pm_reflect(BinarySerializer& visitor) = 0;
			virtual void     pm_reflect(BinaryDeserializer& visitor) = 0;
			virtual void     pm_reflect(TextSerializer& visitor) = 0;
			virtual void     pm_reflect(TextDeserializer& visitor) = 0;
			virtual void     pm_reflect(partial::UpdateVisitor<BinarySerializer>& visitor) = 0;
			virtual void     pm_reflect(partial::UpdateVisitor<BinaryDeserializer>& visitor) = 0;
			virtual void     pm_reflect(partial::UpdateVisitor<TextSerializer>& visitor) = 0;
			virtual void     pm_reflect(partial::UpdateVisitor<TextDeserializer>& visitor) = 0;
			virtual context* get_context() = 0;
			virtual node_id  get_node_id() = 0;
		};

		template <class T>
		class node : public polymorphic_node
		{
			friend class context;

		public:

			template <typename... ARGS>
			node(context* context, const char* type_id, node_id node_id, ARGS&&... value)
				: m_context(context)
				, m_type_id(type_id)
				, m_node_id(node_id)
				, m_value(std::forward<ARGS>(value)...) { }

			virtual ~node() { }

			static type_id get_type_id()
			{
				return T::get_type_id();
			}

			template <typename VISITOR>
			void reflect(VISITOR& visitor)
			{
				visitor("type_id", m_type_id);
				visitor("node_id", m_node_id);
				m_value.reflect(visitor);
			}

			void     pm_reflect(BinarySerializer& visitor) override                           { reflect(visitor); }
			void     pm_reflect(BinaryDeserializer& visitor) override                         { reflect(visitor); }
			void     pm_reflect(TextSerializer& visitor) override                             { reflect(visitor); }
			void     pm_reflect(TextDeserializer& visitor) override                           { reflect(visitor); }
			void     pm_reflect(partial::UpdateVisitor<BinarySerializer>& visitor) override   { reflect(visitor); }
			void     pm_reflect(partial::UpdateVisitor<BinaryDeserializer>& visitor) override { reflect(visitor); }
			void     pm_reflect(partial::UpdateVisitor<TextSerializer>& visitor) override     { reflect(visitor); }
			void     pm_reflect(partial::UpdateVisitor<TextDeserializer>& visitor) override   { reflect(visitor); }
			context* get_context() override                                                   { return m_context; }
			node_id  get_node_id() override                                                   { return m_node_id; }

		private:
			context*    m_context;
			std::string m_type_id;
			node_id     m_node_id;
			T           m_value;
		};

		template<typename Visitor>
		typename void reflect(Visitor &visitor, std::shared_ptr<polymorphic_node>& x)
		{
			x->reflect(visitor);
		}
	}
}