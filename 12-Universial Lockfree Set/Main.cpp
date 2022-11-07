#include "Main.hpp"

int main()
{
	vector <thread*> worker_threads{};

	thread_id = 0;

	for (int num_thread = 1; num_thread <= 16; num_thread *= 2)
	{
		my_set.clear();

		auto start = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			worker_threads.push_back(new thread{ Benchmark, num_thread, i + 1 });
		}

		for (auto pth : worker_threads)
		{
			pth->join();
		}

		auto du = high_resolution_clock::now() - start;

		for (auto pth : worker_threads) delete pth;
		worker_threads.clear();

		cout << num_thread << " Threads   ";
		cout << duration_cast<milliseconds>(du).count() << "ms \n";
		my_set.Print20();
		cout << '\n';
	}

	return 0;
}
