#include "Main.hpp"

constexpr int summary_target = 100000000;
int summary = 0;

template<int Threads_count>
class BakeryLocker
{
public:
	constexpr BakeryLocker()
		: myFlags(), myLabals()
	{}

	inline constexpr void lock(const int id)
	{
		myFlags[id] = true;

		myLabals[id] = std::max(myLabals, myLabals + Threads_count) + 1;

		bool quit = true;
		while (true)
		{
			for (int k = 0; k < Threads_count; k++)
			{
				if (k != id)
				{
					if (myFlags[k] && myLabals[k] < myLabals[id] && k < id)
					{
						quit = false;
					}
				}
			}

			if (quit) break;
		}
	}

	inline constexpr void unlock(const int id)
	{
		myFlags[id] = false;
	}

	std::array<bool, Threads_count> myFlags;
	std::array<int, Threads_count> myLabals;
};

template<int Thread_count>
constexpr void Worker(const int id)
{
	BakeryLocker<Thread_counts> locker{};

	const int local_target = summary_target / Thread_count;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		CasLock(id);
		summary += 2;
		CasUnlock(id);
	}
}

int main()
{
	Worker<2>(0);
	Worker<2>(1);

	return 0;
}
