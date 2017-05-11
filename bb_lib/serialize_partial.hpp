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

		template <typename Serializer, typename key_t = Unused, typename val_t = Unused>
		struct UpdateVisitor
		{
		public:
			UpdateVisitor(Serializer &ser, std::string &tag, UpdateOp op)
				: ser(ser), tag(tag), op(op) { }

			Serializer &ser;
			std::string &tag;
			UpdateOp op;
			key_t key;
			val_t val;

			template <typename T> void acquire_val(T&) { /* used in deserialization, will be filled in */ }
			template <typename T> void acquire_key(T&) { /* used in deserialization, will be filled in */ }

			template <> void acquire_val(val_t& v) { v = val; }
			template <> void acquire_key(key_t& k) { k = key; }

			template <typename T>
			void operator()(const char* found_tag, T& x)
			{
				if (tag.compare(found_tag) == 0)
				{
					switch (op)
					{
					case UpdateOp::Update:
					{
						ser(found_tag, x);
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
				if (tag.compare(found_tag) == 0)
				{
					switch (op)
					{
						case UpdateOp::Update:
						{
							ser(found_tag, x);
							break;
						}
						case UpdateOp::AppendVector:
						{
							T elem; acquire_val(elem);
							ser("elem", elem);
							x.push_back(elem);
							break;
						}
						case UpdateOp::InsertVector:
						{
							size_t index; acquire_key(index);
							T elem; acquire_val(elem);
							ser("index", index);
							ser("elem", elem);
							x.insert(x.begin() + index, elem);
							break;
						}

						case UpdateOp::EraseVector:
						{
							size_t index; acquire_key(index);
							ser("index", index);
							x.erase(x.begin() + index);
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
				if (tag.compare(found_tag) == 0)
				{
					switch (op)
					{
						case UpdateOp::Update:
						{
							ser(found_tag, x);
							break;
						}
						case UpdateOp::InsertMap:
						{
							std::pair<K, V> keyval;
							acquire_key(keyval.first);
							acquire_val(keyval.second);
							ser("elem", keyval);
							x.insert(keyval);
							break;
						}

						case UpdateOp::EraseMap:
						{
							K it; acquire_key(it);
							ser("key", it);
							x.erase(it);
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
					UpdateVisitor<Serializer> visitor(ser, tag, op);
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
		void serialize_member_modify(Serializer& ser, T& object, const char* _tag)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::Update);

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename E>
		void serialize_vector_push_back(Serializer& ser, T& object, const char* _tag, E& elem)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, Unused, E> visitor(ser, tag, UpdateOp::AppendVector);
			visitor.val = elem;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename E>
		void serialize_vector_insert(Serializer& ser, T& object, const char* _tag, size_t index, E& elem)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, size_t, E> visitor(ser, tag, UpdateOp::InsertVector);
			visitor.key = index;
			visitor.val = elem;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_erase(Serializer& ser, T& object, const char* _tag, size_t index)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, size_t, Unused> visitor(ser, tag, UpdateOp::EraseVector);
			visitor.key = index;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_clear(Serializer& ser, T& object, const char* _tag, size_t index)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, Unused, Unused> visitor(ser, tag, UpdateOp::ClearVector);

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename K, typename V>
		void serialize_map_insert(Serializer& ser, T& object, const char* _tag, K& key, V& val)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, K, V> visitor(ser, tag, UpdateOp::InsertMap);
			visitor.key = key;
			visitor.val = val;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename K>
		void serialize_map_erase(Serializer& ser, T& object, const char* _tag, K &key)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, K, Unused> visitor(ser, tag, UpdateOp::EraseMap);
			visitor.key = key;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_map_clear(Serializer& ser, T& object, const char* _tag)
		{
			std::string tag = _tag;

			UpdateVisitor<Serializer, Unused, Unused> visitor(ser, tag, UpdateOp::ClearMap);

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}
	}
}