#pragma once

namespace bb
{
	namespace net
	{
		template <typename Deserializer, typename Serializer>
		class server : public context
		{
		public:
			server(std::shared_ptr<node> root);

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