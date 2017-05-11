#pragma once

namespace bb
{
	namespace net
	{
		class context
		{
		public:
			virtual ~context() { }

		private:
			std::unordered_map<node_id, std::weak_ptr<node>> m_objects;
		};
	}
}