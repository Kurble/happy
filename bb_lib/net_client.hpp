#pragma once

namespace bb
{
	namespace net
	{
		template <typename Deserializer, typename Serializer>
		class client : public context
		{
		public:
			client(Deserializer && svr_in, Serializer && svr_out)
				: m_svr_in(std::move(svr_in))
				, m_svr_out(std::move(svr_out)) { }

			int rpc(node& object, const char* tag)
			{
				std::string rpcTag = tag;
				m_svr_out("rpc", rpcTag);
			}

			template <typename... ARGS, typename E>
			int rpc(node& object, const char* tag, ARGS&&... rest, E& arg)
			{
				int elements = rpc(object, tag, rest...);
				std::string elemTag = "arg" + std::to_string(elements);
				m_svr_out(elemTag.c_str(), arg);
			}

			void update()
			{
				//
			}

		private:
			Deserializer m_svr_in;
			Serializer m_svr_out;
		};
	}
}