#pragma once

#include "serialize.h"

namespace bb
{
	enum class UpdateOp
	{
		Replace,
		Update,
		Append,
		Insert,
		Erase,
		Clear,
	};

	//------------------------------------------------------------------------
	// Update serializer (for net)
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

	template <typename Serializer, typename elem_t>
	struct VectorVisitor
	{
	public:
		VectorVisitor(Serializer &ser, std::string &tag, UpdateOp op)
			: ser(ser), tag(tag), op(op) { }

		Serializer &ser;
		std::string &tag;
		UpdateOp op;
		size_t ind;
		std::tuple<elem_t> obj_wrapped;

		template <typename T>
		void operator()(const char* found_tag, T& x)
		{
			if (tag.compare(found_tag) == 0)
			{
				throw std::exception("std::vector<E> expected");
			}
		}

		template <typename E> void obj(E& obj) { /* used in deserialization, will be filled in */ }

		template <> void obj(elem_t& obj) { std::tie(obj) = obj_wrapped; }

		template <typename E>
		void operator()(const char* found_tag, std::vector<E>& x)
		{
			if (tag.compare(found_tag) == 0)
			{
				switch (op)
				{
					case UpdateOp::Append:
					{
						E e;
						obj(e);
						ser("elem", e);
						x.push_back(e);
						break;
					}
					case UpdateOp::Insert:
					{
						E e;
						obj(e);
						ser("index", ind);
						ser("elem", e);
						x.insert(x.begin() + ind, e);
						break;
					}

					case UpdateOp::Erase:
					{
						ser("index", ind);
						x.erase(x.begin() + ind);
						break;
					}

					case UpdateOp::Clear:
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
	void net_receive(Serializer& ser, T& object)
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

			case UpdateOp::Append:
			case UpdateOp::Insert:
			case UpdateOp::Erase:
			case UpdateOp::Clear:
			{
				ser("tag", tag);
				VectorVisitor<Serializer, int> visitor(ser, tag, op);
				reflect(visitor, object);
			}
		}
	}

	template <typename Serializer, typename T>
	void net_replace(Serializer& ser, T& object)
	{
		UpdateOp op = UpdateOp::Replace;
		ser("op", op);
		ser("root", object);
	}

	template <typename Serializer, typename T> 
	void net_member_modify(Serializer& ser, T& object, const char* _tag)
	{
		std::string tag = _tag;
		UpdateVisitor<Serializer> visitor(ser, tag, UpdateOp::Update);		
		ser("op", visitor.op);
		ser("tag", visitor.tag);
		reflect(visitor, object);
	}

	template <typename Serializer, typename T, typename E> 
	void net_vector_push_back(Serializer& ser, T& object, const char* _tag, E& elem)
	{
		std::string tag = _tag;
		VectorVisitor<Serializer, E> visitor(ser, tag, UpdateOp::Append);
		visitor.obj_wrapped = std::tuple<E>(elem);
		ser("op", visitor.op);
		ser("tag", visitor.tag);
		reflect(visitor, object);
	}

	template <typename Serializer, typename T, typename E> 
	void net_vector_insert(Serializer& ser, T& object, const char* _tag, size_t index, E& elem)
	{
		std::string tag = _tag;
		VectorVisitor<Serializer, E> visitor(ser, tag, UpdateOp::Insert);
		visitor.ind = index;
		visitor.obj_wrapped = std::tuple<E>(elem);
		ser("op", visitor.op);
		ser("tag", visitor.tag);
		reflect(visitor, object);
	}

	template <typename Serializer, typename T>
	void net_vector_erase(Serializer& ser, T& object, const char* _tag, size_t index)
	{
		std::string tag = _tag;
		VectorVisitor<Serializer, int> visitor(ser, tag, UpdateOp::Erase);
		visitor.ind = index;
		ser("op", visitor.op);
		ser("tag", visitor.tag);
		reflect(visitor, object);
	}

	template <typename Serializer, typename T>
	void net_vector_clear(Serializer& ser, T& object, const char* _tag, size_t index)
	{
		std::string tag = _tag;
		VectorVisitor<Serializer, int> visitor(ser, tag, UpdateOp::Clear);
		ser("op", visitor.op);
		ser("tag", visitor.tag);
		reflect(visitor, object);
	}
}