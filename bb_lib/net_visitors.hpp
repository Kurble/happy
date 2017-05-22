#pragma once

namespace bb
{
	namespace net
	{
		enum class update_op
		{
			Replace,
			Update,
			AppendVector,
			InsertVector,
			EraseVector,
			ClearVector,
			InsertMap,
			EraseMap,
			ClearMap,
		};
		
		/*
		 * Server update visitor.
		 * Makes a packet of specific members in a node.
		 * If a member is a node itself, a reference to the parent node is added, and
		 *  the reference that was already there on the old version of the member node is removed.
		 */
		template <typename Serializer>
		struct update_visitor
		{
		public:
			class reference_manager
			{
			public:
				virtual ~reference_manager() = default;
				virtual void unref(std::shared_ptr<polymorphic_node> node) = 0;
				virtual void ref(std::shared_ptr<polymorphic_node> node) = 0;
			};

			update_visitor(Serializer &ser, const char *tag, update_op op, void* key, void* value, reference_manager* refmgr)
				: m_serializer(ser)
				, m_tag(tag) 
				, m_operation(op)
				, m_key(key)
				, m_val(value)
				, m_refmgr(refmgr) { }
			
			Serializer&        m_serializer;
			std::string        m_tag;
			update_op          m_operation;
			void*              m_key;
			void*              m_val;
			int                m_found = 0;
			reference_manager* m_refmgr;
			
			template <typename T> void acquire_key(T& x) { if (m_key) x = *((T*)m_key); }
			template <typename T> void acquire_val(T& x) { if (m_val) x = *((T*)m_val); }

			template <typename T>
			void ref(T& x) { }

			template <typename T>
			void ref(std::shared_ptr<T>& x) 
			{ 
				if (m_refmgr)
				{
					std::shared_ptr<net::polymorphic_node> n = std::dynamic_pointer_cast<net::polymorphic_node, T>(x);
					m_refmgr->ref(n);
				}
			}

			template <typename T>
			void unref(T& x) { }

			template <typename T>
			void unref(std::shared_ptr<T>& x)
			{
				if (m_refmgr)
				{
					std::shared_ptr<net::polymorphic_node> n = std::dynamic_pointer_cast<net::polymorphic_node, T>(x);
					m_refmgr->unref(n);
				}
			}

			template <typename T>
			void operator()(const char* found_tag, T& x)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					if (m_found) throw std::exception("duplicate tag detected");
					m_found++;
					switch (m_operation)
					{
						case update_op::Update:
						{
							unref(x);
							acquire_val(x);
							ref(x);
							m_serializer(found_tag, x);
							break;
						}
						default:
						{
							throw std::exception("invalid type found");
						}
					}
				}
			}

			template <typename T>
			void operator()(const char* found_tag, std::vector<T>& x)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					if (m_found) throw std::exception("duplicate tag detected");
					m_found++;
					switch (m_operation)
					{
						case update_op::Update:
						{
							for (auto &y : x) unref(y);
							acquire_val(x);
							for (auto &y : x) ref(y);
							m_serializer(found_tag, x);
							break;
						}
						case update_op::AppendVector:
						{
							T val; acquire_val(val);
							ref(val);
							m_serializer("val", val);
							x.push_back(val);
							break;
						}
						case update_op::InsertVector:
						{
							size_t key; acquire_key(key);
							T val;      acquire_val(val);
							ref(val);
							m_serializer("ind", key);
							m_serializer("val", val);
							x.insert(x.begin() + key, val);
							break;
						}

						case update_op::EraseVector:
						{
							size_t key; acquire_key(key);
							unref(x[key]);
							m_serializer("ind", key);
							x.erase(x.begin() + key);
							break;
						}

						case update_op::ClearVector:
						{
							for (auto &y : x) unref(y);
							x.clear();
							break;
						}

						default:
						{
							throw std::exception("invalid type found");
							break;
						}
					}
				}
			}

			template <typename K, typename V>
			void operator()(const char* found_tag, std::map<K, V>& x)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					if (m_found) throw std::exception("duplicate tag detected");
					m_found++;
					switch (m_operation)
					{
						case update_op::Update:
						{
							for (auto &y : x) unref(y.second);
							acquire_val(x);
							for (auto &y : x) ref(y.second);
							m_serializer(found_tag, x);
							break;
						}
						case update_op::InsertMap:
						{
							std::pair<K, V> keyval;
							acquire_key(keyval.first);
							acquire_val(keyval.second);
							ref(keyval.second);
							m_serializer("keyval", keyval);
							x.insert(keyval);
							break;
						}

						case update_op::EraseMap:
						{
							K key; acquire_key(key);
							unref(x.at(key));
							m_serializer("key", key);
							x.erase(key);
							break;
						}

						case update_op::ClearMap:
						{
							for (auto &y : x) unref(y.second);
							x.clear();
							break;
						}

						default:
						{
							throw std::exception("invalid type found");
							break;
						}
					}
				}
			}
		};		

		template <typename Serializer>
		struct param_list_deserializer
		{
			param_list_deserializer(Serializer &ser)
				: m_serializer(ser) { }

			Serializer& m_serializer;
			int         m_arg = 0;

			template <typename T, typename... ARGS>
			void get(T& arg, ARGS&... args)
			{
				std::string elemTag = "arg" + std::to_string(m_arg++);
				m_serializer(elemTag.c_str(), arg);
				get(std::forward<ARGS>(args)...);
			}

			template <typename T> arg()
			{
				T x;
				std::string elemTag = "arg" + std::to_string(m_arg++);
				m_serializer(elemTag.c_str(), x);
				return x;
			}

		private:
			void get() { }
		};

		template <typename Serializer>
		struct rpc_visitor
		{
			rpc_visitor(Serializer &ser, const char *tag)
				: m_serializer(ser)
				, m_tag(tag) { }

			Serializer& m_serializer;
			std::string m_tag;
			int         m_found = 0;

			using Params = param_list_deserializer<Serializer>;

			template <typename Fn>
			void operator()(const char* found_tag, Fn deserialize_rpc)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					if (m_found) throw std::exception("duplicate tag detected");
					m_found++;
					
					Params p(m_serializer);
					deserialize_rpc(p);
				}
			}
		};

		template <typename Serializer, typename T>
		void deserialize(Serializer& ser, std::shared_ptr<T>& object)
		{
			std::string tag;
			update_op op;

			ser("op", op);

			std::shared_ptr<polymorphic_node> n = std::dynamic_pointer_cast<polymorphic_node, T>(object);

			switch (op)
			{
				case update_op::Replace:
				{
					ser("val", object);
					break;
				}

				case update_op::Update:
				case update_op::AppendVector:
				case update_op::InsertVector:
				case update_op::EraseVector:
				case update_op::ClearVector:
				case update_op::InsertMap:
				case update_op::EraseMap:
				case update_op::ClearMap:
				{
					ser("tag", tag);
					update_visitor<Serializer> visitor(ser, tag.c_str(), op, nullptr, nullptr, nullptr);
					object->reflect(visitor);
					break;
				}

				default:
				{
					throw std::exception("invalid operation");
				}
			}
		}

		template <typename Serializer, typename T>
		void deserialize(Serializer& ser, T& object)
		{
			std::string tag;
			update_op op;

			ser("op", op);

			switch (op)
			{
				case update_op::Replace:
				{
					ser("val", object);
					break;
				}

				case update_op::Update:
				case update_op::AppendVector:
				case update_op::InsertVector:
				case update_op::EraseVector:
				case update_op::ClearVector:
				case update_op::InsertMap:
				case update_op::EraseMap:
				case update_op::ClearMap:
				{
					ser("tag", tag);
					update_visitor<Serializer> visitor(ser, tag.c_str(), op, nullptr, nullptr, nullptr);
					reflect(visitor, object);
					break;
				}

				default:
				{
					throw std::exception("invalid operation");
				}
			}
		}
	}
}