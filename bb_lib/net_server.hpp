#pragma once

#include <sstream>
#include <algorithm>
#include <Windows.h>

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

			class server_client
			{
			public:
				server_client(std::istream &clt_in, std::ostream &clt_out, std::shared_ptr<polymorphic_node> root)
					: m_clt_in(clt_in)
					, m_clt_out(clt_out)
					, m_root(root) { }

				Deserializer  m_clt_in;
				std::ostream &m_clt_out;
				std::shared_ptr<polymorphic_node> m_root;
			};

			class server_node_base : public context::node_base
			{
			public:
				server_node_base(context* context, const char* type_id, node_id node_id)
					: context::node_base(context, type_id, node_id)
					, m_message_buffer(((server<Deserializer, Serializer>*)context)->m_message_buffer)
					, m_message_serializer(((server<Deserializer, Serializer>*)context)->m_message_serializer) { }

				virtual ~server_node_base() { }

				void resync()
				{
					UpdateOp replace = UpdateOp::Replace;
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", replace);
					m_message_serializer("val", shared_from_this());
					send();
				}

				template <typename T>
				void member_modify(const char *tag, T& x)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::Update, nullptr, (void*)&x);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				template <typename T>
				void vector_push_back(const char *tag, T& x)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::AppendVector, nullptr, (void*)&x);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				template <typename T>
				void vector_insert(const char* tag, size_t index, T& x)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::InsertVector, (void*)&index, (void*)&x);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				void vector_erase(const char* tag, size_t index)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::EraseMap, (void*)&index, nullptr);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				void vector_clear(const char* tag)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::ClearMap, nullptr, nullptr);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				template <typename K, typename V>
				void map_insert(const char* tag, size_t index, K& key, V& value)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::InsertMap, (void*)&key, (void*)&value);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				template <typename K>
				void map_erase(const char* tag, K& key)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::EraseMap, (void*)key, nullptr);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				void map_clear(const char* tag)
				{
					UpdateVisitor<Serializer> visitor(m_message_serializer, tag, UpdateOp::ClearMap, nullptr, nullptr);
					m_message_serializer("node", m_node_id);
					m_message_serializer("op", visitor.m_operation);
					m_message_serializer("tag", visitor.m_tag);
					reflect(visitor);
					send();
				}

				void add_subscriber(std::weak_ptr<server_client> sub)
				{
					m_subscribers.push_back(sub);
				}

			private:
				void send()
				{
					// send buffer to all subscribers
					m_message_buffer.flush();
					OutputDebugString(m_message_buffer.str().c_str());

					m_subscribers.erase(std::remove_if(m_subscribers.begin(), m_subscribers.end(), [&](std::weak_ptr<server_client>& ptr)
					{
						if (auto sub = ptr.lock())
						{
							sub->m_clt_out.write(m_message_buffer.str().c_str(), m_message_buffer.str().length());
							return false;
						}

						return true;
					}), m_subscribers.end());

					// clear contents
					std::ostringstream empty;
					m_message_buffer.swap(empty);
				}

				std::ostringstream& m_message_buffer;
				Serializer&         m_message_serializer;

				friend class server<DESERIALIZER, SERIALIZER>;
				std::vector<std::weak_ptr<server_client>> m_subscribers;
			};

			template <class T>
			using node_type = std::shared_ptr<node<T, server_node_base, std::true_type>>;

			template <class T>
			node_type<T> make_root_node()
			{
				node_type<T> n = std::make_shared<node<T, server_node_base, std::true_type>>(this, T::get_type_id(), m_node_id_counter++);

				m_objects.emplace(n->get_node_id(), n);

				return n;
			}

			template <class T, class P>
			node_type<T> make_node(std::shared_ptr<P> parent)
			{
				node_type<T> n = std::make_shared<node<T, server_node_base, std::true_type>>(this, T::get_type_id(), m_node_id_counter++);

				m_objects.emplace(n->get_node_id(), n);

				n->m_subscribers = std::dynamic_pointer_cast<server_node_base, P>(parent)->m_subscribers;

				return n;
			}

			template <class T>
			node_type<T> cast(std::shared_ptr<polymorphic_node> x)
			{
				return std::dynamic_pointer_cast<node<T, server_node_base, std::true_type>, polymorphic_node>(x);
			}

			template <class T>
			std::shared_ptr<server_client> add_client(std::istream &clt_in, std::ostream &clt_out, node_type<T> clt_root)
			{
				auto client = std::make_shared<server_client>(clt_in, clt_out, clt_root);

				clt_root->add_subscriber(client);
				clt_root->resync();

				return client;
			}

			void update_client(server_client* clt)
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
						node->reflect_rpc(RPCVisitor<Deserializer>(clt->m_clt_in, tag.c_str()));
					}
					else
					{
						throw std::exception("node expired!!!");
					}
				}
			}

		private:
			node_id            m_node_id_counter;
			std::ostringstream m_message_buffer;
			Serializer         m_message_serializer;
		};
	}
}