#pragma once

namespace bb
{
	namespace net
	{
		enum class UpdateOp
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
		
		template <typename Serializer>
		struct UpdateVisitor
		{
		public:
			UpdateVisitor(Serializer &ser, const char *tag, UpdateOp op, void* key = nullptr, void* value = nullptr)
				: m_serializer(ser)
				, m_tag(tag) 
				, m_operation(op)
				, m_key(key)
				, m_val(value) { }

			Serializer& m_serializer;
			std::string m_tag;
			UpdateOp    m_operation;
			void*       m_key;
			void*       m_val;
			int         m_found = 0;
			
			template <typename T> void acquire_key(T& x) { if (m_key) x = *((T*)m_key); }
			template <typename T> void acquire_val(T& x) { if (m_val) x = *((T*)m_val); }

			template <typename T>
			void operator()(const char* found_tag, T& x)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					if (m_found) throw std::exception("duplicate tag detected");
					m_found++;
					switch (m_operation)
					{
						case UpdateOp::Update:
						{
							acquire_val(x);
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
						case UpdateOp::Update:
						{
							acquire_val(x);
							m_serializer(found_tag, x);
							break;
						}
						case UpdateOp::AppendVector:
						{
							T val; acquire_val(val);
							m_serializer("val", val);
							x.push_back(val);
							break;
						}
						case UpdateOp::InsertVector:
						{
							size_t key; acquire_key(key);
							T val;      acquire_val(val);
							m_serializer("ind", key);
							m_serializer("val", val);
							x.insert(x.begin() + key, val);
							break;
						}

						case UpdateOp::EraseVector:
						{
							size_t key; acquire_key(key);
							m_serializer("ind", key);
							x.erase(x.begin() + key);
							break;
						}

						case UpdateOp::ClearVector:
						{
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
						case UpdateOp::Update:
						{
							acquire_val(x);
							m_serializer(found_tag, x);
							break;
						}
						case UpdateOp::InsertMap:
						{
							std::pair<K, V> keyval;
							acquire_key(keyval.first);
							acquire_val(keyval.second);
							m_serializer("keyval", keyval);
							x.insert(keyval);
							break;
						}

						case UpdateOp::EraseMap:
						{
							K key; acquire_key(key);
							m_serializer("key", key);
							x.erase(key);
							break;
						}

						case UpdateOp::ClearMap:
						{
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
		struct ParamListDeserializer
		{
			ParamListDeserializer(Serializer &ser)
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

		private:
			void get() { }
		};

		template <typename Serializer>
		struct RPCVisitor
		{
			RPCVisitor(Serializer &ser, const char *tag)
				: m_serializer(ser)
				, m_tag(tag) { }

			Serializer& m_serializer;
			std::string m_tag;
			int         m_found = 0;

			using Params = ParamListDeserializer<Serializer>;

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
			UpdateOp op;

			ser("op", op);

			std::shared_ptr<polymorphic_node> n = std::dynamic_pointer_cast<polymorphic_node, T>(object);

			switch (op)
			{
				case UpdateOp::Replace:
				{
					ser("val", object);
					break;
				}

				case UpdateOp::Update:
				case UpdateOp::AppendVector:
				case UpdateOp::InsertVector:
				case UpdateOp::EraseVector:
				case UpdateOp::ClearVector:
				case UpdateOp::InsertMap:
				case UpdateOp::EraseMap:
				case UpdateOp::ClearMap:
				{
					ser("tag", tag);
					UpdateVisitor<Serializer> visitor(ser, tag.c_str(), op, nullptr, nullptr);
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
			UpdateOp op;

			ser("op", op);

			switch (op)
			{
				case UpdateOp::Replace:
				{
					ser("val", object);
					break;
				}

				case UpdateOp::Update:
				case UpdateOp::AppendVector:
				case UpdateOp::InsertVector:
				case UpdateOp::EraseVector:
				case UpdateOp::ClearVector:
				case UpdateOp::InsertMap:
				case UpdateOp::EraseMap:
				case UpdateOp::ClearMap:
				{
					ser("tag", tag);
					UpdateVisitor<Serializer> visitor(ser, tag.c_str(), op, nullptr, nullptr);
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