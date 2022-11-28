#include "Main.hpp"

class HISTORY
{
public:
	int op;
	int i_value; // input
	bool o_value; // output

	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re)
	{}
};

ProceduralSkipList my_set{};

void Worker(std::vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < TARGET_SUM / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % 1000;
				my_set.ADD(v);
				break;
			}
			case 1:
			{
				int v = rand() % 1000;
				my_set.REMOVE(v);
				break;
			}
			case 2:
			{
				int v = rand() % 1000;
				my_set.CONTAINS(v);
				break;
			}
		}
	}
}

void worker_check(std::vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < TARGET_SUM / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % 1000;
				history->emplace_back(0, v, my_set.ADD(v));
				break;
			}
			case 1:
			{
				int v = rand() % 1000;
				history->emplace_back(1, v, my_set.REMOVE(v));
				break;
			}
			case 2:
			{
				int v = rand() % 1000;
				history->emplace_back(2, v, my_set.CONTAINS(v));
				break;
			}
		}
	}
}

void check_history(std::array<std::vector<HISTORY>, 16>& history, int num_threads)
{
	std::array<int, 1000> survive{};

	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0)
	{
		std::cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i)
	{
		for (auto& op : history[i])
		{
			if (false == op.o_value) continue;
			if (op.op == 3) continue;

			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}

	for (int i = 0; i < 1000; ++i)
	{
		int val = survive[i];

		if (val < 0)
		{
			std::cout << "The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1)
		{
			std::cout << "The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0)
		{
			if (my_set.CONTAINS(i))
			{
				std::cout << "The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1)
		{
			if (false == my_set.CONTAINS(i))
			{
				std::cout << "The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	std::cout << " OK\n";
}

using namespace std;
using namespace std::chrono;

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		std::vector <std::thread> threads;
		std::array<std::vector <HISTORY>, 16> history{};
		my_set.CLEAR();

		auto start_t = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &history[i], num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.PRINT();
		std::cout << num_threads << "Threads.  Exec Time : " << exec_ms << '\n';

		check_history(history, num_threads);

		std::cout << std::endl;
	}

	std::cout << "===============================================================\n";

	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		std::vector <std::thread> threads;
		std::array<vector <HISTORY>, 16> history{};
		my_set.CLEAR();

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Worker, &history[i], num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.PRINT();
		std::cout << num_threads << "Threads.  Exec Time : " << exec_ms << '\n';

		check_history(history, num_threads);

		std::cout << std::endl;
	}
}
