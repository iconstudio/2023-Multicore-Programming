#include "pch.hpp"
#include "Main.hpp"
#include "OrderedSet.hpp"

OrderedSet my_set{};

constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

void Worker(const int threads_number)
{
	int key = 0;
	for (int i = 0; i < NUM_TEST / threads_number; i++)
	{
		switch (rand() % 3)
		{
			case 0:
			{
				key = rand() % KEY_RANGE;
				
				my_set.Add(key);
			}
			break;

			case 1:
			{
				key = rand() % KEY_RANGE;
				
				my_set.Remove(key);
			}
			break;

			case 2:
			{
				key = rand() % KEY_RANGE;

				my_set.Contains(key);
			}
			break;

			default:
			{
				std::cout << "Error\n";
				exit(-1);
			}
			break;
		}
	}
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		std::vector<std::thread> workers{};
		workers.reserve(th_count);

		auto clock_before = std::chrono::high_resolution_clock::now();

		std::cout << "스레드 수 (" << th_count << ") 시작!\n";

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(Worker, th_count);
		}

		for (auto& th : workers)
		{
			th.join();
		}

		auto clock_after = std::chrono::high_resolution_clock::now();

		auto elapsed_time = clock_after - clock_before;
		auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

		std::cout << "시간: " << ms_time.count() << "ms\n";
		std::cout << "결과: { ";

		int index = 0;
		auto it = my_set.myHead->myNext;
		for (int i = 0; i < 20; i++)
		{
			if (it == my_set.myTail) break;

			std::cout << it->myValue << ", ";

			it = it->myNext;
		}

		my_set.Clear();

		std::cout << "}\n\n";
	}

	return 0;
}
