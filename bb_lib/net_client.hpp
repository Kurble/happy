#pragma once

namespace bb
{
	namespace net
	{
		template <typename DESERIALIZER, typename SERIALIZER>
		class client : private node_resolver<DESERIALIZER>, public context
		{
		public:
			using Deserializer = DESERIALIZER;
			using Serializer = SERIALIZER;

			client(std::istream& svr_in, std::ostream& svr_out)
				: m_svr_in(svr_in, this)
				, m_svr_out(svr_out) { }

			virtual ~client() { }

			using node_base_type = client_node_base<Deserializer, Serializer>;

			template <class T>
			using node_type = std::shared_ptr<node<T, node_base_type, std::false_type>>;

			template <class T, class T2=polymorphic_node>
			node_type<T> cast(std::shared_ptr<T2> x)
			{
				return std::dynamic_pointer_cast<node<T, node_base_type, std::false_type>, T2>(x);
			}

			template <class T>
			void register_node_type()
			{
				m_factories.emplace(std::string(T::node_type()), std::make_shared<sub_factory<T>>());
			}

			void update()
			{
				while (m_svr_in)
				{
					node_id id;
					m_svr_in("node", id);

					if (m_root.get())
					{
						// find appropriate node
						auto it = m_objects.find(id);
						if (it == m_objects.end())
						{
							throw std::exception("node not found");
						}

						if (auto node = it->second.lock())
						{
							deserialize(m_svr_in, node);
						}
						else
						{
							throw std::exception("node expired!!!");
						}
					}
					else
					{
						// we don't know anything yet.. assume root is coming in
						deserialize(m_svr_in, m_root);
					}
				}
			}

			std::shared_ptr<polymorphic_node> get_root() { return m_root; }

		private:
			friend node_base_type;

			class sub_factory_base
			{
			public:
				virtual std::shared_ptr<polymorphic_node> make_node(context*, const char*, node_id) const = 0;
			};

			template <class T>
			class sub_factory : public sub_factory_base
			{
				std::shared_ptr<polymorphic_node> make_node(context* context, const char* type_id, node_id node_id) const override
				{
					return std::make_shared<node<T, node_base_type, std::false_type>>(context, type_id, node_id);
				}
			};

			std::shared_ptr<polymorphic_node> resolve(const char *tid, size_t nid) override
			{
				std::shared_ptr<polymorphic_node> x = nullptr;

				auto it = m_objects.find(nid);
				if (it != m_objects.end()) x = it->second.lock();

				if (x.get() == nullptr)
				{
					// make node using registered factory
					x = m_factories.at(tid)->make_node(this, tid, nid);

					// register the new node
					m_objects[nid] = x;
				}

				if (strcmp(x->get_type_id(), tid)) throw std::exception("node_id/type_id mismatch");

				return x;
			}

			Deserializer m_svr_in;
			Serializer m_svr_out;
			std::shared_ptr<polymorphic_node> m_root;
			std::unordered_map<std::string, std::shared_ptr<sub_factory_base>> m_factories;
		};
	}
}
