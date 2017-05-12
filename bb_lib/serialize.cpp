#include "net.hpp"

#include <fstream>

namespace bb
{
	struct TestStruct
	{
		std::string a;
		float b;

		template <typename Visitor>
		void reflect(Visitor &visit)
		{
			visit("a", a);
			visit("b", b);
		}
	};
	
	struct OuterTestStruct
	{
		std::vector<std::string> henk;
		std::map<int, TestStruct> dude;

		template <typename Visitor>
		void reflect(Visitor& visit)
		{
			visit("henk", henk);
			visit("dude", dude);
		}
	};

	class TestNode
	{
	public:
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
		template <typename VISITOR>
		void reflect(VISITOR& visit)
		{
			TestNode::reflect(visit);
		}

		static bb::net::type_id get_type_id() { return "DerivedTestNode"; }
	};

	void serialize_sanity_check()
	{
		{  
			std::ofstream o("test.bb", std::ios::binary);
			BinarySerializer binarySerialize = o;
			TextSerializer textSerialize = o;

			OuterTestStruct test;
			test.henk = { "aap", "noot", "mies" };
			test.dude[2] = { "derp", 16.0f };
			partial::serialize_replace(textSerialize, test);

			std::string obj = "troll";
			partial::serialize_vector_push_back(textSerialize, test, "henk", obj);
			partial::serialize_vector_erase(textSerialize, test, "henk", 1);

			test.dude[3] = { "lol", 0.0f };
			partial::serialize_member_modify(textSerialize, test, "dude");
		}

		{
			std::ofstream o("test.txt", std::ios::binary);
			std::ifstream i("");
			
			bb::net::client<TextDeserializer, TextSerializer> serverConnection = { TextDeserializer(i), TextSerializer(o) };
			//std::shared_ptr<bb::net::polymorphic_node> rootNode = std::make_shared<bb::net::node<TestNode>>(0, &serverConnection);
			std::shared_ptr<bb::net::polymorphic_node> rootNode = serverConnection.make_node<DerivedTestNode>(0);


			// test polymorphic reflect
			{
				TextSerializer textSerialize = o;
				reflect(textSerialize, rootNode);
			}

			// test an rpc
			{
				int a = 0;
				float b = 0.2f;
				std::string c = "derp";
				serverConnection.rpc(*rootNode, "destroyUniverse", a, b, c);
			}
		}

		{
			std::ifstream i("test.bb", std::ios::binary);
			BinaryDeserializer binaryDeserialize = i;
			TextDeserializer textDeserialize = i;
			
			OuterTestStruct test;
			partial::deserialize(textDeserialize, test);
			partial::deserialize(textDeserialize, test);
			partial::deserialize(textDeserialize, test);
			partial::deserialize(textDeserialize, test);

			void();
		}
	}
}