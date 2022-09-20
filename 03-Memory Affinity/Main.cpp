#include "Main.hpp"

std::atomic_int ax, ay;
volatile int x, y;
std::mutex fxmtx{}, fymtx{};

constexpr auto SIZE = 50000000;
int trace_x[SIZE]{}, trace_y[SIZE]{};

void ThreadFunc0();
void ThreadFunc1();
void MutexFunc0();
void MutexFunc1();
void AtomicFunc0();
void AtomicFunc1();

// Thread 2°³ ½ÇÇà
int main()
{
	std::jthread th1{ MutexFunc0 };
	std::jthread th2{ MutexFunc1 };

	if (th1.joinable())
	{
		th1.join();
	}

	if (th2.joinable())
	{
		th2.join();
	}

	int count = 0;
	for (int i = 0; i < SIZE; ++i)
	{
		if (trace_x[i] == trace_x[i + 1])
		{
			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1])
			{
				if (trace_y[trace_x[i]] != i) continue;
				count++;
			}
		}
	}

	std::cout << "Total Memory Inconsistency: " << count << std::endl;

	return 0;
}

void ThreadFunc0()
{
	for (int i = 0; i < SIZE; i++)
	{
		x = i;

		trace_y[i] = y;
	}
}

void ThreadFunc1()
{
	for (int i = 0; i < SIZE; i++)
	{
		y = i;

		trace_x[i] = x;
	}
}

void AtomicFunc0()
{
	for (int i = 0; i < SIZE; i++)
	{
		ax = i;

		trace_y[i] = ay;
	}
}

void AtomicFunc1()
{
	for (int i = 0; i < SIZE; i++)
	{
		ay = i;

		trace_x[i] = ax;
	}
}

void MutexFunc0()
{
	for (int i = 0; i < SIZE; i++)
	{
		fxmtx.lock();
		x = i;
		fxmtx.unlock();

		fymtx.lock();
		trace_y[i] = y;
		fymtx.unlock();
	}
}

void MutexFunc1()
{
	for (int i = 0; i < SIZE; i++)
	{
		fymtx.lock();
		y = i;
		fymtx.unlock();

		fxmtx.lock();
		trace_x[i] = x;
		fxmtx.unlock();
	}
}
