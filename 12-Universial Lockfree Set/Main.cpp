#include "Main.hpp"

constexpr auto NUM_TEST = 40000;
constexpr auto KEY_RANGE = 1000;

//SeqObject my_set; // 여기서 락이 없어서 그냥 프로그램이 죽는다
//MutexUniversal my_set;
//LFUniversal my_set;
WFUniversal my_set;

void Benchmark(int num_thread, int id)
{
	thread_id = id;

	Invocation invoc{};

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3)
		{
			case 0: invoc.method = M_ADD; break;
			case 1: invoc.method = M_REMOVE; break;
			case 2: invoc.method = M_CONTAINS; break;
		}

		invoc.input = rand() % KEY_RANGE;
		my_set.Apply(invoc);
	}
}

int main()
{
	std::vector<std::thread*> worker_threads{};

	thread_id = 0;

	for (int num_thread = 1; num_thread <= 16; num_thread *= 2)
	{
		my_set.clear();

		const auto start = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			worker_threads.push_back(new std::thread{ Benchmark, num_thread, i + 1 });
		}

		for (auto pth : worker_threads)
		{
			pth->join();
		}

		const auto du = high_resolution_clock::now() - start;

		for (auto pth : worker_threads) delete pth;
		worker_threads.clear();

		cout << num_thread << " Threads: ";
		cout << duration_cast<milliseconds>(du).count() << "ms \n";
		my_set.Print20();

		cout << '\n';
	}

	return 0;
}
