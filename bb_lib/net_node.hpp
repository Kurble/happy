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
		// Node functionality helpers
		struct server_utils_tag { };
		struct client_utils_tag { };
		template <typename SVR, typename CLT>
		class node_utils : public server_utils_tag, public client_utils_tag
		{
		protected:
			using server_node_base = typename SVR::server_node_base;
			using client_node_base = typename CLT::client_node_base;
			virtual ~node_utils() { }			
			server_node_base* svrsvc = nullptr;
			client_node_base* cltsvc = nullptr;
		};
		template <typename SVR>
		class node_utils<SVR, void> : public server_utils_tag
		{
		protected:
			node_utils()
				: svrsvc(*((server_node_base*)nullptr)) { }

			using server_node_base = typename SVR::server_node_base;
			virtual ~node_utils() { }
			server_node_base* svrsvc = nullptr;
		};
		template <typename CLT>
		class node_utils<void, CLT> : public client_utils_tag
		{
		protected:
			node_utils()
				: cltsvc(*((client_node_base*)nullptr)) { }

			using client_node_base = typename CLT::client_node_base;
			virtual ~node_utils() { }
			client_node_base* cltsvc = nullptr;
		};

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
				, USER_IMPL(/*net nodes must have a default constructor*/) 
			{
				link_server<user_type>();
				link_client<user_type>();
			}

			virtual ~node() { }
			
			template <typename VISITOR>
			void reflect(VISITOR& visitor)
			{
				visitor("type_id", m_type_id);
				visitor("node_id", m_node_id);

				if (m_type_id.compare(user_type::get_type_id()))
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
			void     pm_reflect_rpc(RPCVisitor<BinaryDeserializer>& visitor) override { reflect_rpc<gen_reflect_rpc>(visitor); }
			void     pm_reflect_rpc(RPCVisitor<TextDeserializer>& visitor) override   { reflect_rpc<gen_reflect_rpc>(visitor); }

			template <typename IMPL>
			typename std::enable_if<!std::is_base_of<server_utils_tag, IMPL>::value>::type link_server() { /*...*/ }
			
			template <typename IMPL>
			typename std::enable_if<!std::is_base_of<client_utils_tag, IMPL>::value>::type link_client() { /*...*/ }

			template <typename IMPL>
			typename std::enable_if<std::is_base_of<server_utils_tag, IMPL>::value>::type link_server()
			{
				user_type::svrsvc = dynamic_cast<user_type::server_node_base*>(this);
			}

			template <typename IMPL>
			typename std::enable_if<std::is_base_of<client_utils_tag, IMPL>::value>::type link_client()
			{
				user_type::cltsvc = dynamic_cast<user_type::client_node_base*>(this);
			}

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::false_type>::value, void>::type reflect_rpc(VISITOR& visitor)
			{
				throw std::exception("trying to process an RPC while this node does not support RPC processing!");
			}

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::true_type>::value, void>::type reflect_rpc(VISITOR& visitor)
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