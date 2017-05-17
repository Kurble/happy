#pragma once

namespace bb
{
	namespace net
	{
		template <typename DESERIALIZER, typename SERIALIZER>
		class server : public context
		{
		public:
			using Deserializer = DESERIALIZER;
			using Serializer = SERIALIZER;

			server() 
				: m_node_id_counter(0)
				, m_message_serializer(m_message_buffer) {}
			
			using node_base_type = server_node_base<Deserializer, Serializer>;

			template <class T>
			using node_type = std::shared_ptr<node<T, node_base_type, std::true_type>>;

			template <class T>
			node_type<T> make_node()
			{
				node_type<T> n = std::make_shared<node<T, node_base_type, std::true_type>>(this, T::node_type(), m_node_id_counter++);
				m_objects[n->get_node_id()] = n;
				return n;
			}

			template <class T>
			node_type<T> cast(std::shared_ptr<polymorphic_node> x)
			{
				return std::dynamic_pointer_cast<node<T, node_base_type, std::true_type>, polymorphic_node>(x);
			}

			template <class T>
			std::shared_ptr<server_client<Deserializer, Serializer>> add_client(std::istream &clt_in, std::ostream &clt_out)
			{
				node_type<T> clt_root = make_node<T>();

				auto client = std::make_shared<server_client<Deserializer, Serializer>>(clt_in, clt_out, clt_root);

				clt_root->m_owner = client.get();
				clt_root->resync();

				return client;
			}

			void update_client(server_client<Deserializer, Serializer>* clt)
			{
				while (clt->m_clt_in)
				{
					node_id id;
					std::string tag;
					clt->m_clt_in("node", id);
					clt->m_clt_in("rpc", tag);

					// find appropriate node
					auto it = m_objects.find(id);
					if (it == m_objects.end())
					{
						throw std::exception("node not found");
					}

					if (auto node = it->second.lock())
					{
						node->reflect_rpc(rpc_visitor<Deserializer>(clt->m_clt_in, tag.c_str()));
					}
					else
					{
						throw std::exception("node expired!!!");
					}
				}
			}

		private:
			friend node_base_type;

			node_id            m_node_id_counter;
			std::ostringstream m_message_buffer;
			Serializer         m_message_serializer;
		};
	}
}