#include "Main.hpp"

template<size_t Threads_count>
class BakeryLocker
{
public:
	constexpr BakeryLocker()
		: myFlags(), myLabals()
	{}

	void lock(const int id)
	{
		myFlags[id] = true;

		myLabals[id] = std::max(myLabals, myLabals + Threads_count);
	}

	void unlock(const int id)
	{
		myFlags[id] = false;
	}

	std::array<bool, Threads_count> myFlags;
	std::array<int, Threads_count> myLabals;
};

int main()
{


	return 0;
}
