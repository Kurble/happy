#pragma once

#include <Windows.h>
#include <sstream>
#include <algorithm>

namespace bb
{
	namespace net
	{
		template <typename Deserializer, typename Serializer> class server;

		template <typename Deserializer, typename Serializer>
		class server_node_base : public context::node_base, private update_visitor<Serializer>::reference_manager
		{
		public:
			using server_type = server<Deserializer, Serializer>;
			using client_type = server_client<Deserializer, Serializer>;
			using self_type   = server_node_base<Deserializer, Serializer>;

			server_node_base(context* context, const char* type_id, node_id node_id)
				: context::node_base(context, type_id, node_id)
				, m_message_buffer(((server_type*)context)->m_message_buffer)
				, m_message_serializer(((server_type*)context)->m_message_serializer)
				, m_owner(nullptr) { }

			virtual ~server_node_base() { }

			void resync()
			{
				// resync only serializes what we already have, 
				//  no reference resolving required because they are server side only.

				update_op replace = update_op::Replace;
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", replace);
				m_message_serializer("val", shared_from_this());
				send();
			}

			template <typename T>
			void member_modify(const char *tag, T& x)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::Update, nullptr, (void*)&x, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			template <typename T>
			void vector_push_back(const char *tag, T& x)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::AppendVector, nullptr, (void*)&x, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			template <typename T>
			void vector_insert(const char* tag, size_t index, T& x)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::InsertVector, (void*)&index, (void*)&x, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			void vector_erase(const char* tag, size_t index)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::EraseMap, (void*)&index, nullptr, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			void vector_clear(const char* tag)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::ClearMap, nullptr, nullptr, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			template <typename K, typename V>
			void map_insert(const char* tag, size_t index, K& key, V& value)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::InsertMap, (void*)&key, (void*)&value, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			template <typename K>
			void map_erase(const char* tag, K& key)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::EraseMap, (void*)key, nullptr, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}

			void map_clear(const char* tag)
			{
				update_visitor<Serializer> visitor(m_message_serializer, tag, update_op::ClearMap, nullptr, nullptr, this);
				m_message_serializer("node", m_node_id);
				m_message_serializer("op", visitor.m_operation);
				m_message_serializer("tag", visitor.m_tag);
				reflect(visitor);
				send();
			}
			
		private:
			friend server_type;

			void unref(std::shared_ptr<polymorphic_node> node) override
			{
				if (auto x = std::dynamic_pointer_cast<self_type, polymorphic_node>(node))
				{
					// remove my reference from x
					for (auto i = x->m_referenced_by.begin(); i != x->m_referenced_by.end(); ++i)
					{
						auto j = i->lock();
						if (j.get() == this)
						{
							x->m_referenced_by.erase(i);
							return;
						}
					}
				}
				else
				{
					if (node.get()) throw std::exception("unable to cast to server node");
				}
			}

			void ref(std::shared_ptr<polymorphic_node> node) override
			{
				if (auto x = std::dynamic_pointer_cast<self_type, polymorphic_node>(node))
				{
					// add my reference to x
					for (auto &i : x->m_referenced_by)
					{
						auto j = i.lock();
						if (j.get() == this) return;
					}

					x->m_referenced_by.push_back(std::dynamic_pointer_cast<self_type, polymorphic_node>(shared_from_this()));
				}
				else
				{
					if (node.get()) throw std::exception("unable to cast to server node");
				}
			}
			
			template <typename Fn>
			void visit_subscribers(Fn visit)
			{
				if (m_owner) visit(m_owner);

				m_referenced_by.erase(std::remove_if(m_referenced_by.begin(), m_referenced_by.end(), [&](std::weak_ptr<self_type>& ptr)
				{
					if (auto ref = ptr.lock())
					{
						ref->visit_subscribers(visit);
						return false;
					}

					return true;
				}), m_referenced_by.end());
			}
			
			void send()
			{
				// send buffer to all subscribers
				m_message_buffer.flush();
				OutputDebugString(m_message_buffer.str().c_str());

				visit_subscribers([&](client_type* subscriber)
				{
					subscriber->m_clt_out.write(m_message_buffer.str().c_str(), m_message_buffer.str().length());
					subscriber->m_clt_out.flush();
				});

				// clear contents
				std::ostringstream empty;
				m_message_buffer.swap(empty);
			}

			std::ostringstream& m_message_buffer;

			Serializer&         m_message_serializer;

			client_type*        m_owner;

			std::vector<std::weak_ptr<self_type>> m_referenced_by;
		};
	}
}