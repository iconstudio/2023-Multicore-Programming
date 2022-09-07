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
		CPU �ھ� ����
		1. std::cin ������ ���μ��� ���߱�
		2. �۾� �����ڿ��� ���μ��� ã��
		3. ���� ���μ����� CPU ��ȣ���� �ھ� 1���� ���ϱ�
	*/
	worker_summary_simple();

	std::cout.width(6);
	std::cout << "(1) �հ�: " << sum << "\n";

	sum = 0;

	std::thread th1{ worker_summary_mutex };
	std::thread th2{ worker_summary_mutex };

	if (th1.joinable()) th1.join();
	if (th2.joinable()) th2.join();
	
	std::cout << "(2) �հ�: " << sum << "\n";

	return 0;
}
