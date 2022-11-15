#include "Main.hpp"

using namespace std;
using namespace chrono;

constexpr int target_summary = 10000000;

Stack my_set{};

void Worker(const unsigned& number_threads)
{
	for (unsigned i = 0; i < target_summary / number_threads; ++i)
	{
		if (rand() % 2 || i < 1000 / number_threads)
		{
			my_set.Push(i);
		}
		else
		{
			int result = my_set.Pop();
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		vector<thread> threads;

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Worker, num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.Print();
		cout << '\n' << num_threads << " Threads. Exec Time: " << exec_ms << endl;
		cout << endl;

		my_set.Clear();
	}
}
