#include "pch.hpp"
#include "Main.hpp"
#include "OrderedSet.hpp"

OrderedSet my_set{};

void Worker(const int threads_number, const int index)
{
	for (int i = 0; i < 100; i++)
	{
		my_set.Push(i);
	}

	for (int i = 0; i < 100; i++)
	{
		my_set.Push(i);
	}
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		std::vector<std::jthread> workers{};
		workers.reserve(th_count);

		auto clock_before = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(Worker, th_count, i);
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
		std::cout << "시간: " << ms_time << "\n";

		my_set.Clear();
	}

	return 0;
}
