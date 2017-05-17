#pragma once

namespace bb
{
	namespace net
	{
		template <typename Deserializer, typename Serializer>
		class server_node_base;

		template <typename Deserializer, typename Serializer>
		class server;

		template <typename Deserializer, typename Serializer>
		class server_client
		{
		public:
			using node_type = server_node_base<Deserializer, Serializer>;
			using server_type = server<Deserializer, Serializer>;

			server_client(std::istream &clt_in, std::ostream &clt_out, std::shared_ptr<polymorphic_node> root)
				: m_clt_in(clt_in)
				, m_clt_out(clt_out)
				, m_root(root) { }

			template <typename T>
			node<T, node_type, std::true_type>* get_root() { return dynamic_cast<node<T, node_type, std::true_type>*>(m_root.get()); }

		private:
			friend node_type;
			friend server_type;

			Deserializer  m_clt_in;
			std::ostream &m_clt_out;
			std::shared_ptr<polymorphic_node> m_root;
		};
	}
}