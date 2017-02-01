#pragma once

#include <memory>
#include <functional>

namespace happy
{
	template <typename Type>
	class AutoDestroy
	{
		struct _autoDestroy
		{
			Type x;

			std::function<void(const Type& x)> deleter;

			_autoDestroy(Type x, std::function<void(const Type& x)> deleter)
			{
				this->x = x;
				this->deleter = deleter;
			}
			~_autoDestroy()
			{
				deleter(x);
			}
		};

		std::shared_ptr<_autoDestroy> val;

	public:
		AutoDestroy()
			: val(nullptr) { }

		AutoDestroy(Type x, std::function<void(const Type& x)> deleter)
			: val(std::make_shared<_autoDestroy>(x, deleter)) { }
		
		AutoDestroy(const AutoDestroy &copy)
			: val(copy.val) { }

		operator Type&() { return *val.get(); }
	};
}