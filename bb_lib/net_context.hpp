#pragma once

namespace bb
{
	namespace net
	{
		class context
		{
		public:
			context() { }
			virtual ~context() { }

			class node_base : public polymorphic_node
			{
			protected:
				node_base(context* context, const char* type_id, node_id node_id)
					: m_context(context)
					, m_type_id(type_id)
					, m_node_id(node_id) { }

				virtual ~node_base() { }

				context*    m_context;
				std::string m_type_id;
				node_id     m_node_id;
			};

		protected:
			std::unordered_map<node_id, std::weak_ptr<polymorphic_node>> m_objects;
		};
	}
}