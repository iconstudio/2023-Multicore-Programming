#include "Main.hpp"

volatile bool done = false;
volatile int* bound;

int error{};

// 0, -1 반복
void ThreadFunc1()
{
	for (int j = 0; j <= 25000000; ++j)
	{
		*bound = -(1 + *bound);
	}

	done = true;
}

// Thread 1 감시
void ThreadFunc2()
{
	while (!done)
	{
		int v = *bound;

		if ((v != 0) && (v != -1))
		{
			error++;
		}

		std::cout << std::hex << v << '\n';
	}
}

int main()
{
	int bar[32]{};
	long long addr = reinterpret_cast<long long>(&bar[30]);

	addr = (addr / 64) * 64;
	addr -= 1;

	bound = reinterpret_cast<int*>(addr);
	*bound = 0;

	std::jthread th1{ ThreadFunc1 };
	std::jthread th2{ ThreadFunc2 };

	if (th1.joinable())
	{
		th1.join();
	}

	if (th2.joinable())
	{
		th2.join();
	}

	std::cout << "Total Memory Inconsistency: " << error << std::endl;

	return 0;
}

