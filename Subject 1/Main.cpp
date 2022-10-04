#include "Main.hpp"

constexpr int summary_target = 100000000;
volatile int summary = 0;

class BakeryLocker
{
public:
	constexpr BakeryLocker(int th_count)
		: threadsNumber(th_count)
		, myFlags(th_count, false), myLabels()
	{
		//myLabels.reserve(th_count);
		myLabels.resize(th_count);
	}

	inline void lock(const int id)
	{
		myFlags[id] = true;

		auto& it = std::max(myLabels.begin(), myLabels.end());
		myLabels[id] = *it + 1;

		for (int k = 0; k < threadsNumber; k++)
		{
			if (k != id)
			{
				while (myFlags[k]);

				while (myLabels[k] != 0
					&& (myLabels[k] < myLabels[id]
					|| (myLabels[k] == myLabels[id] && k < id)));
			}
		}
	}

	inline void unlock(const int id)
	{
		myFlags[id] = false;
	}

	const int threadsNumber;
	std::vector<bool> myFlags;
	std::vector<std::atomic_int> myLabels;
};

void Worker(BakeryLocker& locker, const int id)
{
	const int local_target = summary_target / locker.threadsNumber;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		locker.lock(id);
		summary += 2;
		locker.unlock(id);
	}
}

void LocalWorker(BakeryLocker& locker, const int id)
{
	const int local_target = summary_target / locker.threadsNumber;
	const int local_times = local_target / 2;

	int local_sum = 0;
	for (int i = 0; i < local_times; i++)
	{
		local_sum += 2;
	}

	locker.lock(id);
	summary += local_sum;
	locker.unlock(id);
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		BakeryLocker locker{ th_count };

		std::vector<std::thread> workers{};
		workers.reserve(th_count);

		auto clock_before = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(Worker, std::ref(locker), i);
		}

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
