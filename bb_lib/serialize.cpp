#include "net.hpp"

#include <fstream>

namespace bb
{
	using Clt = bb::net::client<TextDeserializer, TextSerializer>;
	using Svr = bb::net::server<TextDeserializer, TextSerializer>;

	class TestNode : public net::node_utils<Svr, Clt>
	{
	public:
		virtual ~TestNode() { }

		std::string test_a;
		std::string test_b;
		std::string test_c;

		template <typename VISITOR>
		void reflect(VISITOR& visit)
		{
			visit("test_a", test_a);
			visit("test_b", test_b);
			visit("test_c", test_c);
		}

		static bb::net::type_id get_type_id() { return "TestNode"; }
	};

	class DerivedTestNode : public TestNode
	{
	public:
		virtual ~DerivedTestNode() { }

		std::vector<std::shared_ptr<TestNode>> test_recurse;

		template <typename VISITOR>
		void reflect(VISITOR& visit)
		{
			TestNode::reflect(visit);
			visit("test_recurse", test_recurse);
		}

		static bb::net::type_id get_type_id() { return "DerivedTestNode"; }
	};

	class SvrDerivedTestNode : public DerivedTestNode
	{
	public:
		virtual ~SvrDerivedTestNode() { }

		template <typename VISITOR>
		void reflect_rpc(VISITOR& visit)
		{
			visit("petKittens", [&](VISITOR::Params& params)
			{
				int count;
				params.get(count);

				std::string val = std::to_string(count);
				svrsvc->member_modify("test_a", val);
			});
		}
	};

	class CltDerivedTestNode : public DerivedTestNode
	{
	public:
		virtual ~CltDerivedTestNode() { }
	};

	void serialize_sanity_check()
	{
		// example: client logs in (svr side)
		{
			Svr server;

			std::ifstream i("");
			std::ofstream o("server-test.txt", std::ios::binary);

			auto connected = server.make_root_node<SvrDerivedTestNode>();
			connected->test_a = "var a";
			connected->test_b = "var b";
			connected->test_c = "var c";
			auto client = server.add_client(i, o, connected);

			auto sub = server.make_node<SvrDerivedTestNode>(connected);
			connected->vector_push_back("test_recurse", sub);

			sub->member_modify("test_a", std::string("test"));
		}

		// example: client logs in (clt side)
		{
			std::ifstream i("server-test.txt", std::ios::binary);
			std::ofstream o("client-test.txt", std::ios::binary);

			Clt serverConnection = { i, o };
			serverConnection.register_node_type<TestNode>();
			serverConnection.register_node_type<CltDerivedTestNode>();
			serverConnection.update();

			auto root = serverConnection.cast<CltDerivedTestNode>(serverConnection.get_root());

			int count = 5;
			root->rpc("petKittens", count);
		}

		// example: client from earlier sent an rpc
		{
			Svr server;

			std::ifstream i("client-test.txt", std::ios::binary);
			std::ofstream o("server-test.txt", std::ios::binary);

			auto connected = server.make_root_node<SvrDerivedTestNode>();
			connected->test_a = "var a";
			connected->test_b = "var b";
			connected->test_c = "var c";
			auto client = server.add_client(i, o, connected);

			server.update_client(client.get());
		}
	}
}