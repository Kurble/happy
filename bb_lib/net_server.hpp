#pragma once

namespace bb
{
	namespace net
	{
		class server_client
		{
		public:
			//
		};

		template <class T>
		class server_node
		{
		public:
			T m_node;

			std::vector<std::shared_ptr<server_client>> m_subscribers;
		};

		template <typename Deserializer, typename Serializer>
		class server : public context
		{
		public:
			server();

			void add_client(node_id level, Deserializer && clt_in, Serializer && clt_out)
			{
				//
			}

			void update()
			{
				//
			}
		};
	}
}