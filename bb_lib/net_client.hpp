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

			template <typename... ARGS>
			void rpc(node& object, const char* tag, ARGS&... args)
			{
				node_id nodeId = object.get_node_id();
				std::string rpcTag = tag;
				m_svr_out("node", nodeId);
				m_svr_out("rpc", rpcTag);
				serializeArgs(0, args...);
			}

			void update()
			{
				//
			}

		private:
			void serializeArgs(int argNumber) { }

			template <typename T, typename... ARGS>
			void serializeArgs(int argNumber, T& arg, ARGS&... rest)
			{
				std::string elemTag = "arg" + std::to_string(argNumber);
				m_svr_out(elemTag.c_str(), arg);

				serializeArgs(argNumber + 1, rest...);
			}			

			Deserializer m_svr_in;
			Serializer m_svr_out;
		};
	}
}
