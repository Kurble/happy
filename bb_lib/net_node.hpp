#pragma once

#include "serialize.hpp"

namespace bb
{
	namespace net
	{
		//------------------------------------------------------------------------------------------------------------
		// Forward defines
		class context;

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
			virtual type_id  get_type_id() = 0;
		private:
			virtual void     pm_reflect(BinarySerializer& visitor) = 0;
			virtual void     pm_reflect(BinaryDeserializer& visitor) = 0;
			virtual void     pm_reflect(TextSerializer& visitor) = 0;
			virtual void     pm_reflect(TextDeserializer& visitor) = 0;
			virtual void     pm_reflect(update_visitor<BinarySerializer>& visitor) = 0;
			virtual void     pm_reflect(update_visitor<BinaryDeserializer>& visitor) = 0;
			virtual void     pm_reflect(update_visitor<TextSerializer>& visitor) = 0;
			virtual void     pm_reflect(update_visitor<TextDeserializer>& visitor) = 0;
			virtual void     pm_reflect_rpc(rpc_visitor<BinaryDeserializer>& visitor) = 0;
			virtual void     pm_reflect_rpc(rpc_visitor<TextDeserializer>& visitor) = 0;
		};
				
		//------------------------------------------------------------------------------------------------------------
		// Node functionality helpers
		struct server_utils_tag { };
		struct client_utils_tag { };
		template <typename SVR, typename CLT>
		class node_utils : public server_utils_tag, public client_utils_tag
		{
		protected:
			using server_node_base = typename SVR::node_base_type;
			using client_node_base = typename CLT::node_base_type;
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

			using server_node_base = typename SVR::node_base_type;
			virtual ~node_utils() { }
			server_node_base* svrsvc = nullptr;
		};
		template <typename CLT>
		class node_utils<void, CLT> : public client_utils_tag
		{
		protected:
			node_utils()
				: cltsvc(*((client_node_base*)nullptr)) { }

			using client_node_base = typename CLT::node_base_type;
			virtual ~node_utils() { }
			client_node_base* cltsvc = nullptr;
		};

		//------------------------------------------------------------------------------------------------------------
		// Node wrapper. Wraps around a node object, thereby declaring it as a node.
		// Dispatches all visitors to the wrapped object
		template <typename USER_IMPL, typename NODE_IMPL, typename GEN_REFLECT_RPC>
		class node : public USER_IMPL, public NODE_IMPL
		{
		public:
			using usr_type = USER_IMPL;
			using gen_rpc = GEN_REFLECT_RPC;

			node(context* context, const char* type_id, node_id node_id)
				: NODE_IMPL(context, type_id, node_id)
				, USER_IMPL(/*net nodes must have a default constructor*/) 
			{
				_link_svr<usr_type>();
				_link_clt<usr_type>();
			}

			virtual ~node() { }

			/*
			 * !!! bb::net::node implementation MANUAL !!!
			 *
			 * 1) Nodes require a reflect function template that lists all [relevant] members of the node.
			 *    A reflect function template can be declared as follows:
			 *
			 *    template <typename VISITOR>
			 *    void reflect(VISITOR& visit)
			 *    {
			 *        visit("foo", m_foo);
			 *        visit("bar", m_bar);
			 *    }
			 *
			 *    Note that members are visited using a /tag/, this is required so that the reflect system
			 *     can find the same member later on. It is important that the order of tags remains the same.
			 *
			 * 2) Nodes require a static function that returns their type as a unique string.
			 *    This type is used to construct matching nodes of the proper type.
			 *    If you have dedicated server and client node types,
			 *     make sure their reflect(...) and node_type() match!
			 *
			 * 3) Server-side nodes require a reflect_rpc function template that lists all supported RPCs.
			 *    A reflect_rpc function template can be declared as follows:
			 *   
			 *    template <typename VISITOR>
			 *    void reflect_rpc(VISITOR& visitor)
			 *    {
			 *        visitor("rpcName", [&](VISITOR::Params& params)
			 *        {
			 *            int a, b, c;
			 *            params.get(a, b, c);
			 *        });
			 *    }
			 */

			context* get_context() override                                            { return m_context; }
			node_id  get_node_id() override                                            { return m_node_id; }
			type_id  get_type_id() override                                            { return usr_type::node_type(); }
		private:
			void     pm_reflect(BinarySerializer& visitor) override                    { usr_type::reflect(visitor); }
			void     pm_reflect(BinaryDeserializer& visitor) override                  { usr_type::reflect(visitor); }
			void     pm_reflect(TextSerializer& visitor) override                      { usr_type::reflect(visitor); }
			void     pm_reflect(TextDeserializer& visitor) override                    { usr_type::reflect(visitor); }
			void     pm_reflect(update_visitor<BinarySerializer>& visitor) override    { usr_type::reflect(visitor); }
			void     pm_reflect(update_visitor<BinaryDeserializer>& visitor) override  { usr_type::reflect(visitor); }
			void     pm_reflect(update_visitor<TextSerializer>& visitor) override      { usr_type::reflect(visitor); }
			void     pm_reflect(update_visitor<TextDeserializer>& visitor) override    { usr_type::reflect(visitor); }
			void     pm_reflect_rpc(rpc_visitor<BinaryDeserializer>& visitor) override { _rpc<gen_rpc>(visitor); }
			void     pm_reflect_rpc(rpc_visitor<TextDeserializer>& visitor) override   { _rpc<gen_rpc>(visitor); }

			template <typename IMPL>
			typename std::enable_if<!std::is_base_of<server_utils_tag, IMPL>::value>::type _link_svr() { /*...*/ }
			
			template <typename IMPL>
			typename std::enable_if<!std::is_base_of<client_utils_tag, IMPL>::value>::type _link_clt() { /*...*/ }

			template <typename IMPL>
			typename std::enable_if<std::is_base_of<server_utils_tag, IMPL>::value>::type _link_svr()
			{
				usr_type::svrsvc = dynamic_cast<usr_type::server_node_base*>(this);
			}

			template <typename IMPL>
			typename std::enable_if<std::is_base_of<client_utils_tag, IMPL>::value>::type _link_clt()
			{
				usr_type::cltsvc = dynamic_cast<usr_type::client_node_base*>(this);
			}

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::false_type>::value, void>::type _rpc(VISITOR& visitor)
			{
				throw std::exception("unsupported operation");
			}

			template <typename GEN, typename VISITOR>
			typename std::enable_if<std::is_same<GEN, std::true_type>::value, void>::type _rpc(VISITOR& visitor)
			{
				// If you get compiler errors, your node might be invalid. Scroll up for a manual.
				usr_type::reflect_rpc(visitor);
			}
		};
	}
}