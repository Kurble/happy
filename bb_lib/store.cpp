#include "store.h"

#include <fstream>
#include <cctype>
#include <vector>

namespace bb
{
	using namespace std;

	store::store() { }

	store::store(istream &stream)
	{
		load(stream);
	}

	store::store(const fs::path &file)
	{
		load(file);
	}

	void store::load(const fs::path &fromFile)
	{
		ifstream fin(fromFile);
		load(fin);
	}

	vector<store::val> loadList(istream &s);
	std::string takeLiteral(istream &s)
	{
		std::string result;

		while (!s.eof())
		{
			int peeked = s.peek();

			if (peeked == '[' ||
				peeked == ']' ||
				peeked == '{' ||
				peeked == '}' ||
				std::isspace(peeked)) return result;

			result += s.get();
		}

		return result;
	}
	store::val parseval(istream &s)
	{
		char next = s.peek();

		if (next == '\"')
		{
			s.ignore();

			char buf[1024];
			s.getline(buf, 1024, '\"');

			return string(buf);
		}
		else if (next == '{')
		{
			s.ignore();
			return store(s);
		}
		else if (next == '[')
		{
			s.ignore();
			return loadList(s);
		}
		else if (next == '#')
		{
			s.ignore();
			std::string lit = takeLiteral(s);
			char *p;
			int64_t i = strtoll(lit.c_str(), &p, 16);

			if (*p) throw exception("malformed hex literal");

			return i;
		}
		else
		{
			std::string lit = takeLiteral(s);

			char* p;
			int64_t i = strtoll(lit.c_str(), &p, 10);
			if (*p)
			{
				float f = strtof(lit.c_str(), &p);
				if (*p) throw exception("malformed number literal");

				return f;
			}
			else
			{
				return i;
			}
		}
	}
	vector<store::val> loadList(istream &s)
	{
		vector<store::val> list;
		while (!s.eof())
		{
			while (std::isspace(s.peek())) s.ignore();

			if (s.peek() == ']')
			{
				s.ignore();
				return list;
			}

			if (s.peek() == '}')
			{
				throw exception("invalid termination of list");
			}

			list.emplace_back(parseval(s));
		}

		return list;
	}
	void store::load(istream &s)
	{
		while (!s.eof())
		{
			while (std::isspace(s.peek())) s.ignore();

			if (s.peek() == '}')
			{
				s.ignore();
				return;
			}
			if (s.peek() == ']')
			{
				s.ignore();
				throw exception("invalid termination of data");
			}

			string tag = takeLiteral(s);

			while (std::isspace(s.peek())) s.ignore();

			m_Data.emplace(tag, parseval(s));
		}
	}
	void store::save(const fs::path &toFile)
	{
		ofstream fout(toFile);
		save(fout);
	}
	void store::save(ostream &toStream)
	{
		//
	}

	void store::visit(std::function<void(const std::string &key, const val &elem)> visitor) const
	{
		for (const auto &elem : m_Data)
		{
			visitor(elem.first, elem.second);
		}
	}

	bool store::exists(const string &key) const
	{
		return m_Data.find(key) != m_Data.end();
	}

	float store::getFieldF(const string &key) const
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::Float && i->second.type != val::Int) throw exception("error in store: expected different type");
			return i->second.f;
		}
		else throw exception("error in store: expected field not found");
	}
	int64_t store::getFieldI(const string &key) const
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::Float && i->second.type != val::Int) throw exception("error in store: expected different type");
			return i->second.i;
		}
		else throw exception("error in store: expected field not found");
	}
	string store::getFieldS(const string &key) const
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::String) throw exception("error in store: expected different type");
			return i->second.s;
		}
		else throw exception("error in store: expected field not found");
	}
	const store& store::getFieldD(const string &key) const
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::Data) throw exception("error in store: expected different type");
			return i->second.d;
		}
		else throw exception("error in store: expected field not found");
	}
	const vector<store::val>& store::getFieldL(const string &key) const
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::List) throw exception("error in store: expected different type");
			return i->second.l;
		}
		else throw exception("error in store: expected field not found");
	}
	store& store::getFieldD(const string &key)
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::Data) throw exception("error in store: expected different type");
			return i->second.d;
		}
		else throw exception("error in store: expected field not found");
	}
	vector<store::val>& store::getFieldL(const string &key)
	{
		auto i = m_Data.find(key);
		if (i != m_Data.end())
		{
			if (i->second.type != val::List) throw exception("error in store: expected different type");
			return i->second.l;
		}
		else throw exception("error in store: expected field not found");
	}

	void store::setFieldF(const string &key, const float &val)
	{
		m_Data.emplace(key, val);
	}
	void store::setFieldI(const string &key, const int64_t &val)
	{
		m_Data.emplace(key, val);
	}
	void store::setFieldS(const string &key, const string &val)
	{
		m_Data.emplace(key, val);
	}
	void store::setFieldD(const string &key, const store &val)
	{
		m_Data.emplace(key, val);
	}
	void store::setFieldL(const string &key, const vector<val> &val)
	{
		m_Data.emplace(key, val);
	}
}