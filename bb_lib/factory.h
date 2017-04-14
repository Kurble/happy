#pragma once

#include <memory>
#include <string>
#include <map>

#define CLASS_WITH_FACTORY(T, base) class T; namespace { bb::spec_factory<T, base>::auto_register _create##T(#T); } class T: public base

#define CLASS_WITH_FACTORY_NAMED(T, base, name) class T; namespace { bb::spec_factory<T, base>::auto_register _create##T(name); } class T: public base

namespace bb
{
	template <typename base> struct generic_factory
	{
		virtual ~generic_factory() = default;
		virtual std::unique_ptr<base> create() = 0;
	};

	template <typename T, typename base> struct spec_factory : public generic_factory<base>
	{
		struct auto_register
		{
			auto_register(const char *name)
			{
				factory<base>::instance().registerClass<T>(name);
			}
		};

		std::unique_ptr<base> create() override
		{
			return std::make_unique<T>();
		}
	};

	template<typename base>
	class factory
	{
	public:
		template <class T> void registerClass(const char* name)
		{
			m_Factories.emplace(name, std::make_shared<spec_factory<T, base>>());
		}

		std::shared_ptr<generic_factory<base>>& retrieveFactory(std::string &name)
		{
			return m_Factories.at(name);
		}

		std::unique_ptr<base> create(std::string name)
		{
			return m_Factories.at(name)->create();
		}

		static factory<base>& instance()
		{
			if (!m_Instance)
			{
				m_Instance = make_unique<factory<base>>();
			}

			return *m_Instance.get();
		}

	private:
		static std::unique_ptr<factory<base>> m_Instance;

		std::map<std::string, std::shared_ptr<generic_factory<base>>> m_Factories;
	};

	template <typename base> std::unique_ptr<factory<base>> factory<base>::m_Instance;
}