#include "net.hpp"

#include <fstream>

namespace bb
{
	using Clt = bb::net::client<TextDeserializer, TextSerializer>;
	using Svr = bb::net::server<TextDeserializer, TextSerializer>;

	class TestNode
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

	void serialize_sanity_check()
	{
		{
			std::ofstream o("server-test.txt", std::ios::binary);
			std::ifstream i("");

			Svr server;

			// example: client logs in
			{
				auto connected = server.make_root_node<DerivedTestNode>();
				connected->test_a = "var a";
				connected->test_b = "var b";
				connected->test_c = "var c";
				server.add_client(i, o, connected);

				auto sub = server.make_node<DerivedTestNode>(connected);
				connected->vector_push_back("test_recurse", sub);

				sub->member_modify("test_a", std::string("test"));
			}
		}

		{
			std::ofstream o("client-test.txt", std::ios::binary);
			std::ifstream i("server-test.txt", std::ios::binary);

			Clt serverConnection = { i, o };
			serverConnection.register_node_type<TestNode>();
			serverConnection.register_node_type<DerivedTestNode>();
			serverConnection.update();

			auto root = serverConnection.cast<DerivedTestNode>(serverConnection.get_root());

			int count = 5;
			root->rpc("petKittens", count);
		}
	}
}