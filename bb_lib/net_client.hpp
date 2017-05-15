#pragma once

namespace bb
{
	namespace net
	{
		template <typename DESERIALIZER, typename SERIALIZER>
		class client : private node_factory_base<DESERIALIZER>, public context
		{
		public:
			using Deserializer = DESERIALIZER;
			using Serializer = SERIALIZER;

			client(std::istream& svr_in, std::ostream& svr_out)
				: m_svr_in(svr_in, this)
				, m_svr_out(svr_out) { }

			virtual ~client() { }

			class client_node_base : public context::node_base
			{
			public:
				client_node_base(context* context, const char* type_id, node_id node_id)
					: context::node_base(context, type_id, node_id)
					, m_svr_out(((client<Deserializer, Serializer>*)context)->m_svr_out) { }

				virtual ~client_node_base() { }

				template <typename... ARGS>
				void rpc(const char* tag, ARGS&... args)
				{
					std::string rpcTag = tag;
					m_svr_out("node", m_node_id);
					m_svr_out("rpc", rpcTag);
					serializeArgs(0, args...);
				}

			private:
				void serializeArgs(int argNumber) { }

				template <typename T, typename... ARGS>
				void serializeArgs(int argNumber, T& arg, ARGS&... rest)
				{
					std::string elemTag = "arg" + std::to_string(argNumber);
					m_svr_out(elemTag.c_str(), arg);
					serializeArgs(argNumber + 1, rest...);
				}

				Serializer& m_svr_out;
			};

			template <class T>
			using node_type = std::shared_ptr<node<T, client_node_base, std::false_type>>;

			template <class T>
			node_type<T> cast(std::shared_ptr<polymorphic_node> x)
			{
				return std::dynamic_pointer_cast<node<T, client_node_base, std::false_type>, polymorphic_node>(x);
			}

			template <class T>
			void register_node_type()
			{
				m_factories.emplace(std::string(T::get_type_id()), std::make_shared<sub_factory<T>>());
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
							partial::deserialize(m_svr_in, node);
						}
						else
						{
							throw std::exception("node expired!!!");
						}
					}
					else
					{
						// we don't know anything yet.. assume root is coming in
						partial::deserialize(m_svr_in, m_root);
					}
				}
			}

			std::shared_ptr<polymorphic_node> get_root() { return m_root; }

		private:
			class sub_factory_base
			{
			public:
				virtual std::shared_ptr<polymorphic_node> make_node(context*, const char*, node_id, Deserializer&) const = 0;
			};

			template <class T>
			class sub_factory : public sub_factory_base
			{
				std::shared_ptr<polymorphic_node> make_node(context* context, const char* type_id, node_id node_id, Deserializer& deserializer) const override
				{
					auto n = std::make_shared<node<T, client_node_base, std::false_type>>(context, type_id, node_id);
					n->__user_reflect(deserializer);
					return n;
				}
			};

			std::shared_ptr<polymorphic_node> make_node(Deserializer& deserializer) override
			{
				std::string type_id;
				node_id     node_id;

				// find out what kind of node we're going to make
				deserializer("type_id", type_id);
				deserializer("node_id", node_id);

				// make node using registered factory
				auto n = m_factories.at(type_id)->make_node(this, type_id.c_str(), node_id, deserializer);

				// register the new node
				m_objects.emplace(node_id, n);

				// done!
				return n;
			}

			Deserializer m_svr_in;
			Serializer m_svr_out;
			std::shared_ptr<polymorphic_node> m_root;
			std::unordered_map<std::string, std::shared_ptr<sub_factory_base>> m_factories;
		};
	}
}
