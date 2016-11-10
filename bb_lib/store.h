#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <filesystem>

namespace bb
{
	namespace fs = std::experimental::filesystem;
	
	class store
	{
	public:
		store();
		store(std::istream &fromStream);
		store(const fs::path &fromFile);

		void load(const fs::path &fromFile);
		void load(std::istream &fromStream);
		void save(const fs::path &toFile);
		void save(std::ostream &toStream);
		
		struct val;

		void visit(std::function<void(const std::string &key, const val &elem)> &visitor) const;

		bool exists(const std::string &key) const;

		float                   getFieldF(const std::string &key) const;
		int64_t                 getFieldI(const std::string &key) const;
		std::string             getFieldS(const std::string &key) const;
		const store&            getFieldD(const std::string &key) const;
		const std::vector<val>& getFieldL(const std::string &key) const;
		store&                  getFieldD(const std::string &key);
		std::vector<val>&       getFieldL(const std::string &key);
		void                    setFieldF(const std::string &key, const float &val);
		void                    setFieldI(const std::string &key, const int64_t &val);
		void                    setFieldS(const std::string &key, const std::string &val);
		void                    setFieldD(const std::string &key, const store &val);
		void                    setFieldL(const std::string &key, const std::vector<val> &val);

	private:
		std::map<std::string, val> m_Data;
	};

	struct store::val
	{
		enum { Float, Int, String, Data, List } type;

		float f;
		int64_t i;
		std::string s;
		store d;
		std::vector<val> l;

		val(const val &copy)
		{
			type = copy.type;
			switch (type)
			{
			case Float:  f = copy.f; i = copy.i; break;
			case Int:    f = copy.f; i = copy.i; break;
			case String: s = copy.s; break;
			case Data:   d = copy.d; break;
			case List:   l = copy.l; break;
			}
		}
		val(const float &f) : type(Float), f(f), i((int)f) { }
		val(const int64_t &i) : type(Int), i(i), f((float)i) { }
		val(const std::string &s) : type(String), s(s) { }
		val(const store &d) : type(Data), d(d) { }
		val(const std::vector<val> &l) : type(List), l(l) { }
	};
}