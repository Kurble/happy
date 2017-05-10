#include "serialize.h"
#include "serialize_net.h"

#include <fstream>

namespace bb
{
	struct TestStruct
	{
		std::string a;
		float b;
	};

	template <typename Visitor>
	void serialize(TestStruct& x, Visitor& visit)
	{
		visit("a", x.a);
		visit("b", x.b);
	}

	struct OuterTestStruct
	{
		std::vector<std::string> henk;
		std::map<int, TestStruct> dude;
	};

	template <typename Visitor>
	void serialize(OuterTestStruct& x, Visitor& visit)
	{
		visit("henk", x.henk);
		visit("dude", x.dude);
	}

	void serialize_sanity_check()
	{
		{
			std::ofstream o("test.bb", std::ios::binary);
			BinarySerializer binarySerialize = o;

			OuterTestStruct test;
			test.henk = { "aap", "noot", "mies" };
			test.dude[2] = { "derp", 16.0f };
			serialize(test, binarySerialize);

			test.dude[3] = { "lol", 0.0f };

			tag_modified(test, "dude", o);

			std::ofstream ot("test.txt");
			TextSerializer textSerialize = ot;
			serialize(test, textSerialize);
		}

		{
			std::ifstream i("test.bb", std::ios::binary);
			OuterTestStruct test;

			{
				BinaryDeserializer binaryDeserialize = i;
				serialize(test, binaryDeserialize);
			}

			{
				UpdateDeserializer updatePack(i);
				serialize(test, updatePack);
			}

			void();
		}

	}
}