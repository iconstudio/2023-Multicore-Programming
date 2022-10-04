#//include "Main.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <algorithm>

constexpr int summary_target = 10000000;
volatile int summary = 0;
std::mutex summary_mtx{};

class BakeryLocker
{
public:
	constexpr BakeryLocker(int th_count)
		: threadsNumber(th_count)
		, myFlags(th_count, false), myLabels(th_count, 0)
	{}

	inline constexpr void lock(const int id)
	{
		myFlags[id] = true;

		auto& it = std::max(myLabels.begin(), myLabels.end());
		myLabels[id] = *it + 1;

		for (int k = 0; k < threadsNumber; k++)
		{
			if (k != id)
			{
				while (myFlags[k]);

				while (myLabels[k] != 0 && (myLabels[k] < myLabels[id] || (myLabels[k] == myLabels[id] && k < id)));
			}
		}
	}

	inline constexpr void unlock(const int id)
	{
		myFlags[id] = false;
	}

	const int threadsNumber;
	std::vector<bool> myFlags;
	std::vector<int> myLabels;
};

constexpr void BakeryWorker(const int th_count, const int id)
{
	BakeryLocker locker{ th_count };

	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		locker.lock(id);
		summary += 2;
		locker.unlock(id);
	}
}

constexpr void LocalWorker(const int th_count, const int id)
{
	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	int local_sum = 0;
	for (int i = 0; i < local_times; i++)
	{
		local_sum += 2;
	}

	summary += local_sum;
}

constexpr void PlainWorker(const int th_count, const int id)
{
	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		summary += 2;
	}
}

void MutexWorker(const int th_count, const int id)
{
	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		summary_mtx.lock();
		summary += 2;
		summary_mtx.unlock();
	}
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		std::vector<std::thread> workers{};
		workers.reserve(th_count);

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(BakeryWorker, th_count, i);
		}

		auto clock_before = std::chrono::high_resolution_clock::now();

		for (auto& th : workers)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		auto clock_after = std::chrono::high_resolution_clock::now();

		auto elapsed_time = clock_after - clock_before;
		auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

		std::cout << "스레드 수: " << th_count << "개\n";
		std::cout << "합계: " << summary << "\n";
		std::cout << "시간: " << ms_time.count() << "ms\n";

		summary = 0;
	}

	return 0;
}
