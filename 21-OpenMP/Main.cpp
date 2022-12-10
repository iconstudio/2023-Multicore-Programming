#include "Main.hpp"

constexpr int TARGET_SUMMARY = 100000000;
volatile int summary = 0;

void Worker()
{
	int tid = 0, nthreads = 0;
	int i, times = 0;

#pragma omp parallel private(i, tid, nthreads, times), shared(summary)
	{
		tid = omp_get_thread_num();
		nthreads = omp_get_num_threads();

		times = TARGET_SUMMARY / nthreads;

#pragma omp critical
		for (i = 0; i < times / 2; i++)
		{
			// 여기에 #pragma omp critical 넣는 것보다 빠르다.
			summary += 2;
		}
	}
}

int main()
{
	int nthreads, tid;

	/* Fork a team of threads with each thread having a private tid variable */
#pragma omp parallel private(tid)
	{
	/* Obtain and print thread id */
		tid = omp_get_thread_num();
		std::cout << "Hello World from thread (" << tid << ")\n";

		/* Only master thread does this */
		if (tid == 0)
		{
			nthreads = omp_get_num_threads();

			std::cout << "Number of threads = " << nthreads << std::endl;
		}
	} /* All threads join master thread and terminate */

	Worker();

	std::cout << "Summary: " << summary << "\n";

	return 0;
}
