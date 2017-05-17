#pragma once

namespace bb
{
	namespace net
	{
		template <typename Deserializer, typename Serializer>
		class client_node_base : public context::node_base
		{
		public:
			client_node_base(context* context, const char* type_id, node_id node_id)
				: context::node_base(context, type_id, node_id)
				, m_svr_out(((client<Deserializer, Serializer>*)context)->m_svr_out) { }

			virtual ~client_node_base() { }

			template <typename... ARGS>
			void rpc(const char* tag, ARGS&... args)
			{
				std::string rpcTag = tag;
				m_svr_out("node", m_node_id);
				m_svr_out("rpc", rpcTag);
				serializeArgs(0, args...);
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

			Serializer& m_svr_out;
		};
	}
}