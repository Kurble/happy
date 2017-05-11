#include "serialize.h"

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

	void serialize_sanity_check()
	{
		{  
			std::ofstream o("test.bb", std::ios::binary);
			BinarySerializer binarySerialize = o;
			TextSerializer textSerialize = o;

			OuterTestStruct test;
			test.henk = { "aap", "noot", "mies" };
			test.dude[2] = { "derp", 16.0f };
			net_replace(textSerialize, test);

			std::string obj = "troll";
			net_vector_push_back(textSerialize, test, "henk", obj);
			net_vector_erase(textSerialize, test, "henk", 1);

			test.dude[3] = { "lol", 0.0f };
			net_member_modify(textSerialize, test, "dude");
		}

		{
			std::ifstream i("test.bb", std::ios::binary);
			BinaryDeserializer binaryDeserialize = i;
			TextDeserializer textDeserialize = i;
			
			OuterTestStruct test;
			net_receive(textDeserialize, test);
			net_receive(textDeserialize, test);
			net_receive(textDeserialize, test);
			net_receive(textDeserialize, test);

			void();
		}
	}
}