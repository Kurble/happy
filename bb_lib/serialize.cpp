#include "serialize.h"

#include <fstream>

namespace bb
{
	void serialize(std::string& value, BinarySerializer &s)
	{
		size_t size = value.size();
		serialize(size, s);
		s.put(value.data(), value.size());
	}

	void serialize(std::string& value, BinaryDeserializer &s)
	{
		size_t size;
		serialize(size, s);
		value.resize(size);
		s.get(&value[0], value.size());
	}

	struct TestStruct
	{
		std::string a;
		float b;
	};

	template <typename Serializer>
	void serialize(TestStruct& x, Serializer& s)
	{
		serialize(x.a, s);
		serialize(x.b, s);
	}

	void serialize_sanity_check()
	{
		{
			std::ofstream o("test.bb", std::ios::binary);
			BinarySerializer test = o;


			std::vector<std::string> vectorOfStrings = { "aap", "noot", "mies" };

			std::map<int, TestStruct> mapOfTestStruct;
			mapOfTestStruct[2] = { "derp", 16.0f };

			serialize(vectorOfStrings, test);
			serialize(mapOfTestStruct, test);
		}

		{
			std::ifstream i("test.bb", std::ios::binary);
			BinaryDeserializer test = i;

			std::vector<std::string> vectorOfStrings;
			std::map<int, TestStruct> mapOfTestStruct;

			serialize(vectorOfStrings, test);
			serialize(mapOfTestStruct, test);

			void();
		}

	}
}