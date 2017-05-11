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

			struct Unused2 { };
		}

		template <typename Serializer>
		struct UpdateVisitor
		{
		public:
			UpdateVisitor(Serializer &ser, std::string &tag, UpdateOp op)
				: ser(ser), tag(tag), op(op) { }

			Serializer &ser;
			std::string &tag;
			UpdateOp op;

			template <class T>
			void operator()(const char* found_tag, T& x)
			{
				if (tag.compare(found_tag) == 0)
					ser(found_tag, x);
			}
		};

		template <typename Serializer, typename elem_t = Unused>
		struct VectorVisitor
		{
		public:
			VectorVisitor(Serializer &ser, std::string &tag, UpdateOp op)
				: ser(ser), tag(tag), op(op) { }

			Serializer &ser;
			std::string &tag;
			UpdateOp op;
			size_t ind;
			elem_t elem;

			template <typename T>
			void operator()(const char* found_tag, T& x)
			{
				if (tag.compare(found_tag) == 0)
				{
					throw std::exception("std::vector<E> expected");
				}
			}

			template <typename E> void acquire(E& e) { /* used in deserialization, will be filled in */ }

			template <> void acquire(elem_t& e) { e = elem; }

			template <typename E>
			void operator()(const char* found_tag, std::vector<E>& x)
			{
				if (tag.compare(found_tag) == 0)
				{
					switch (op)
					{
					case UpdateOp::AppendVector:
					{
						E e;
						acquire(e);
						ser("elem", e);
						x.push_back(e);
						break;
					}
					case UpdateOp::InsertVector:
					{
						E e;
						acquire(e);
						ser("index", ind);
						ser("elem", e);
						x.insert(x.begin() + ind, e);
						break;
					}

					case UpdateOp::EraseVector:
					{
						ser("index", ind);
						x.erase(x.begin() + ind);
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
		};

		template <typename Serializer, typename key_t = Unused2, typename val_t = Unused>
		struct MapVisitor
			{
			public:
				MapVisitor(Serializer &ser, std::string &tag, UpdateOp op)
					: ser(ser), tag(tag), op(op) { }

				Serializer &ser;
				std::string &tag;
				UpdateOp op;
				key_t key;
				val_t val;

				template <typename T>
				void operator()(const char* found_tag, T& x)
				{
					if (tag.compare(found_tag) == 0)
					{
						throw std::exception("std::map<key_t, val_t> expected");
					}
				}

				template <typename E> void acquire(E& e) { /* used in deserialization, will be filled in */ }

				template <> void acquire(key_t& k) { k = key; }
				template <> void acquire(val_t& v) { v = val; }

				template <typename K, typename V>
				void operator()(const char* found_tag, std::map<K, V>& x)
				{
					if (tag.compare(found_tag) == 0)
					{
						switch (op)
						{
						case UpdateOp::InsertMap:
						{
							std::pair<K, V> keyval;
							acquire(keyval.first);
							acquire(keyval.second);
							ser("elem", keyval);
							x.insert(keyval);
							break;
						}

						case UpdateOp::EraseMap:
						{
							K key;
							acquire(key);
							ser("key", key);
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
							throw std::exception("invalid operation specified");
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
			{
				ser("tag", tag);
				UpdateVisitor<Serializer> visitor(ser, tag, op);
				reflect(visitor, object);
				break;
			}

			case UpdateOp::AppendVector:
			case UpdateOp::InsertVector:
			case UpdateOp::EraseVector:
			case UpdateOp::ClearVector:
			{
				ser("tag", tag);
				VectorVisitor<Serializer> visitor(ser, tag, op);
				reflect(visitor, object);
				break;
			}

			case UpdateOp::InsertMap:
			case UpdateOp::EraseMap:
			case UpdateOp::ClearMap:
			{
				ser("tag", tag);
				MapVisitor<Serializer> visitor(ser, tag, op);
				reflect(visitor, object);
				break;
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

			VectorVisitor<Serializer, E> visitor(ser, tag, UpdateOp::AppendVector);
			visitor.elem = elem;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename E>
		void serialize_vector_insert(Serializer& ser, T& object, const char* _tag, size_t index, E& elem)
		{
			std::string tag = _tag;

			VectorVisitor<Serializer, E> visitor(ser, tag, UpdateOp::InsertVector);
			visitor.ind = index;
			visitor.elem = elem;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_erase(Serializer& ser, T& object, const char* _tag, size_t index)
		{
			std::string tag = _tag;

			VectorVisitor<Serializer> visitor(ser, tag, UpdateOp::EraseVector);
			visitor.ind = index;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_vector_clear(Serializer& ser, T& object, const char* _tag, size_t index)
		{
			std::string tag = _tag;

			VectorVisitor<Serializer> visitor(ser, tag, UpdateOp::ClearVector);

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T, typename K, typename V>
		void serialize_map_insert(Serializer& ser, T& object, const char* _tag, K& key, V& val)
		{
			std::string tag = _tag;

			MapVisitor<Serializer, K, V> visitor(ser, tag, UpdateOp::InsertMap);
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

			MapVisitor<Serializer, K> visitor(ser, tag, UpdateOp::EraseMap);
			visitor.key = key;

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}

		template <typename Serializer, typename T>
		void serialize_map_clear(Serializer& ser, T& object, const char* _tag)
		{
			std::string tag = _tag;

			MapVisitor<Serializer> visitor(ser, tag, UpdateOp::ClearMap);

			ser("op", visitor.op);
			ser("tag", visitor.tag);
			reflect(visitor, object);
		}
	}
}