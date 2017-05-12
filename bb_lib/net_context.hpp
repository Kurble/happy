#pragma once

namespace bb
{
	namespace net
	{
		class context
		{
		public:
			virtual ~context() { }

			template <class T, typename... ARGS>
			std::shared_ptr<node<T>> make_node(node_id id, ARGS&&... args)
			{
				return std::make_shared<node<T>>(this, node<T>::get_type_id(), id, T(std::forward<ARGS>(args)...));
			}

		private:
			std::unordered_map<node_id, std::weak_ptr<polymorphic_node>> m_objects;
		};
	}
}