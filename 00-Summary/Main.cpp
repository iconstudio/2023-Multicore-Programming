#include "Main.hpp"

int sum{};
std::mutex sum_mtx{};

void worker_summary_simple()
{
	for (auto i = 0; i < 5000000; ++i)
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
	std::scoped_lock lock{ sum_mtx };
	for (auto i = 0; i < 2500000; ++i)
	{
		sum += 2;
	}
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

	std::cout.width(6);
	std::cout << "(1) 합계: " << sum << "\n";

	sum = 0;

	std::thread th1{ worker_summary_mutex };
	std::thread th2{ worker_summary_mutex };

	if (th1.joinable()) th1.join();
	if (th2.joinable()) th2.join();
	
	std::cout << "(2) 합계: " << sum << "\n";

	return 0;
}
