#pragma once

#include "serialize.hpp"

namespace bb
{
	namespace net
	{
		//------------------------------------------------------------------------------------------------------------
		// Forward defines
		class context;

		template <typename USER_IMPL, typename NODE_IMPL, typename GEN_REFLECT_RPC> class node;

		//------------------------------------------------------------------------------------------------------------
		// Typedefs
		typedef size_t node_id;

		typedef const char* type_id;

		//------------------------------------------------------------------------------------------------------------
		// Node with unknown type
		class polymorphic_node : public std::enable_shared_from_this<polymorphic_node>
		{
		public:
			virtual ~polymorphic_node() { }

			template <typename VISITOR>
			inline void reflect(VISITOR& visitor)
			{
				this->pm_reflect(visitor);
			}

			template <typename VISITOR>
			inline void reflect_rpc(VISITOR& visitor)
			{
				this->pm_reflect_rpc(visitor);
			}

			virtual context* get_context() = 0;
			virtual node_id  get_node_id() = 0;
		private:
			virtual void     pm_reflect(BinarySerializer& visitor) = 0;
			virtual void     pm_reflect(BinaryDeserializer& visitor) = 0;
			virtual void     pm_reflect(TextSerializer& visitor) = 0;
			virtual void     pm_reflect(TextDeserializer& visitor) = 0;
			virtual void     pm_reflect(UpdateVisitor<BinarySerializer>& visitor) = 0;
			virtual void     pm_reflect(UpdateVisitor<BinaryDeserializer>& visitor) = 0;
			virtual void     pm_reflect(UpdateVisitor<TextSerializer>& visitor) = 0;
			virtual void     pm_reflect(UpdateVisitor<TextDeserializer>& visitor) = 0;
			virtual void     pm_reflect_rpc(RPCVisitor<BinaryDeserializer>& visitor) = 0;
			virtual void     pm_reflect_rpc(RPCVisitor<TextDeserializer>& visitor) = 0;
		};
				
		//------------------------------------------------------------------------------------------------------------
		// Node wrapper. Wraps around a node object, thereby declaring it as a node.
		// Dispatches all visitors to the wrapped object
		template <typename USER_IMPL, typename NODE_IMPL, typename GEN_REFLECT_RPC>
		class node : public USER_IMPL, public NODE_IMPL
		{
		public:
			using user_type = USER_IMPL;
			using gen_reflect_rpc = GEN_REFLECT_RPC;

			node(context* context, const char* type_id, node_id node_id)
				: NODE_IMPL(context, type_id, node_id)
				, USER_IMPL(/*net nodes must have a default constructor*/) { }

			virtual ~node() { }
			
			template <typename VISITOR>
			void reflect(VISITOR& visitor)
			{
				visitor("type_id", m_type_id);
				visitor("node_id", m_node_id);

				if (m_type_id.compare(USER_IMPL::get_type_id()))
				{
					throw std::exception("different type_id expected!");
				}

				user_type::reflect(visitor);
			}

			context* get_context() override                                           { return m_context; }
			node_id  get_node_id() override                                           { return m_node_id; }
		private:
			void     pm_reflect(BinarySerializer& visitor) override                   { reflect(visitor); }
			void     pm_reflect(BinaryDeserializer& visitor) override                 { reflect(visitor); }
			void     pm_reflect(TextSerializer& visitor) override                     { reflect(visitor); }
			void     pm_reflect(TextDeserializer& visitor) override                   { reflect(visitor); }
			void     pm_reflect(UpdateVisitor<BinarySerializer>& visitor) override    { reflect(visitor); }
			void     pm_reflect(UpdateVisitor<BinaryDeserializer>& visitor) override  { reflect(visitor); }
			void     pm_reflect(UpdateVisitor<TextSerializer>& visitor) override      { reflect(visitor); }
			void     pm_reflect(UpdateVisitor<TextDeserializer>& visitor) override    { reflect(visitor); }
			void     pm_reflect_rpc(RPCVisitor<BinaryDeserializer>& visitor) override { __reflect_rpc<GEN_REFLECT_RPC>(visitor); }
			void     pm_reflect_rpc(RPCVisitor<TextDeserializer>& visitor) override   { __reflect_rpc<GEN_REFLECT_RPC>(visitor); }

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::false_type>::value, void>::type __reflect_rpc(VISITOR& visitor)
			{
				throw std::exception("trying to process an RPC while this node does support RPC processing!");
			}

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::true_type>::value, void>::type __reflect_rpc(VISITOR& visitor)
			{
				/*
				* If your node is constructed using bb::net::server::make_node, it requires a reflect_rpc method to be present.
				* If not, a compiler error will be generated right here!
				*
				* Example reflect_rpc:
				*
				* template <typename VISITOR>
				* void reflect_rpc(VISITOR& visitor)
				* {
				*     visitor("rpcName", [&](VISITOR::Params& params)
				*     {
				*         int a, b, c;
				*         params.get(a, b, c);
				*     });
				* }
				*/
				user_type::reflect_rpc(visitor);
			}
		};
	}
}