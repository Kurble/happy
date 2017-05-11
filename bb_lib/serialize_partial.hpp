#pragma once

namespace bb
{
	namespace partial
	{
		namespace
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

			struct Unused { };
		}

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
			
			template <typename T> void acquire_key(T& x) { if (m_key) x = *((T*)m_key); }
			template <typename T> void acquire_val(T& x) { if (m_val) x = *((T*)m_val); }

			template <typename T>
			void operator()(const char* found_tag, T& x)
			{
				if (m_tag.compare(found_tag) == 0)
				{
					switch (m_operation)
					{
						case UpdateOp::Update:
						{
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
					switch (m_operation)
					{
						case UpdateOp::Update:
						{
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
							throw std::exception("invalid operation specified");
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
					switch (m_operation)
					{
						case UpdateOp::Update:
						{
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
					ser("root", object);
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

		template <typename Serializer, typename T>
		void serialize_replace(Serializer& ser, T& object)
		{
			UpdateOp op = UpdateOp::Replace;
			ser("op", op);
			ser("root", object);
		}

		template <typename Serializer, typename T>
		void serialize_member_modify(Serializer& ser, T& object, const char* tag)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::Update);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename E>
		void serialize_vector_push_back(Serializer& ser, T& object, const char* tag, E& elem)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::AppendVector, nullptr, (void*)&elem);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename E>
		void serialize_vector_insert(Serializer& ser, T& object, const char* tag, size_t index, E& elem)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::InsertVector, (void*)&index, (void*)&elem);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_erase(Serializer& ser, T& object, const char* tag, size_t index)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::EraseVector, (void*)&index, nullptr);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_clear(Serializer& ser, T& object, const char* tag, size_t index)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::ClearVector, nullptr, nullptr);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename K, typename V>
		void serialize_map_insert(Serializer& ser, T& object, const char* tag, K& key, V& val)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::InsertMap, (void*)&key, (void*)&val);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename K>
		void serialize_map_erase(Serializer& ser, T& object, const char* tag, K &key)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::EraseMap, (void*)&key, nullptr);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_map_clear(Serializer& ser, T& object, const char* tag)
		{
			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::ClearMap, nullptr, nullptr);
			ser("op", visitor.m_operation);
			ser("tag", visitor.m_tag);
			reflect(visitor, object);
		}
	}
}