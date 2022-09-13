#include "Main.hpp"

constexpr int target_summary = 1000000000;

volatile int sum{};
volatile int sum_array[32]{};
std::atomic<int> atomic_sum{};

std::mutex mutex_sum{};

void worker_summary_simple()
{
	const int mine = (target_summary / 1) / 2;

	for (auto i = 0; i < mine; ++i)
	{
		sum += 2;
	}
}

void worker_summary_duo()
{
	for (auto i = 0; i < 2500000; ++i)
	{
		sum += 2;
	}
}

void worker_summary_mutex()
{
	std::scoped_lock lock{ mutex_sum };
	for (auto i = 0; i < 2500000; ++i)
	{
		sum += 2;
	}
}

void worker_summary_multiple(const int numbers, const int addition)
{
	const int mine = (target_summary / numbers) / addition;

	std::scoped_lock lock{ mutex_sum };
	for (auto i = 0; i < mine; ++i)
	{
		sum += addition;
	}
}

void worker_summary_atomic(const int numbers, const int addition)
{
	const int mine = (target_summary / numbers) / addition;

	for (auto i = 0; i < mine; ++i)
	{
		// atomic_sum = atomic_sum + addition; 은 틀린 결과가 나온다!
		atomic_sum += addition;
	}
}

void worker_summary_local(const int numbers, const int addition)
{
	const int mine = (target_summary / numbers) / addition;

	int local_sum{};
	for (auto i = 0; i < mine; ++i)
	{
		local_sum += addition;
	}

	//std::scoped_lock lock{ mutex_sum };
	sum += local_sum;
}

void worker_summary_array(const int numbers, const size_t index)
{
	const int mine = (target_summary / numbers);
	
	for (auto i = 0; i < mine; ++i)
	{
		sum_array[index]++;
	}

	//std::scoped_lock lock{ mutex_sum };
	sum += sum_array[index];
}

void worker_summary_array_fixed(const int numbers, const size_t index)
{
	const int target = (target_summary / numbers);
	const size_t mine = index * 2;

	for (auto i = 0; i < target; ++i)
	{
		sum_array[mine]++;
	}

	//std::scoped_lock lock{ mutex_sum };
	sum += sum_array[mine];
}

int main()
{
	/*
		CPU 코어 제한
		1. std::cin 등으로 프로세스 멈추기
		2. 작업 관리자에서 프로세스 찾기
		3. 멈춘 프로세스의 CPU 선호도를 코어 1개로 정하기
	*/
	worker_summary_simple();

	std::cout << std::setw(6) << "(1) 합계: " << sum << "\n";

	sum = 0;
	std::vector<std::thread> threads_pool{};

	//
	auto clock_before = std::chrono::high_resolution_clock::now();

	//std::thread th1{ worker_summary_mutex };
	//std::thread th2{ worker_summary_mutex };

	constexpr int th_count = 1;

	for (auto i = 0; i < th_count; i++)
	{
		threads_pool.emplace_back(worker_summary_local, th_count, 2);
	}

	for (auto i = 0; i < th_count; i++)
	{
		auto& th = threads_pool.at(i);

		if (th.joinable())
		{
			th.join();
		}
	}

	auto clock_after = std::chrono::high_resolution_clock::now();

	auto elapsed_time = clock_after - clock_before;
	auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

	std::cout << "(2) 합계: " << sum << "\n";
	std::cout << "(2) 시간: " << ms_time << "\n";

	sum = 0;
	threads_pool.clear();

	//
	clock_before = std::chrono::high_resolution_clock::now();
	
	for (auto i = 0; i < th_count; i++)
	{
		threads_pool.emplace_back(worker_summary_array_fixed, th_count, i);
	}

	for (auto i = 0; i < th_count; i++)
	{
		auto& th = threads_pool.at(i);

		if (th.joinable())
		{
			th.join();
		}
	}

	clock_after = std::chrono::high_resolution_clock::now();

	elapsed_time = clock_after - clock_before;
	ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

	std::cout << "(3) 합계: " << sum << "\n";
	std::cout << "(3) 시간: " << ms_time << "\n";

	return 0;
}
