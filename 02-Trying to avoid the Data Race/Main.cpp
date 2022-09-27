#include "Main.hpp"

constexpr int summary_target = 100000000;

int summary = 0;
volatile int victim = 0;
volatile bool flag[2] = { false, false };

std::atomic_int ga_summary = 0;
std::atomic_int ga_victim = 0;
std::atomic_bool ga_flags[] = { false, false };

/// <summary>
/// 0: 아무도 락을 얻지 않았음.
/// 1: 누군가 락을 얻어서 임계 영역을 실행 중.
/// </summary>
volatile int cas_locker = 0;

inline bool CAS(std::atomic_int* addr, int expected, int new_val)
{
	return std::atomic_compare_exchange_strong(addr, &expected, new_val);
}

inline bool CAS(volatile int* addr, int expected, int new_val)
{
	return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic_int*>(addr), &expected, new_val);
}

/// <summary>
/// cas_locker가 1이면 0이 될 때 까지 대기함.
/// cas_locker가 0이면 원자적으로 1로 바꾸고, 반환한다.
/// </summary>
/// <param name="id"></param>
inline void CasLock(const int id)
{
	while (!CAS(&cas_locker, 0, 1));
}

/// <summary>
/// 원자적으로 cas_locker를 0으로 바꾼다.
/// </summary>
/// <param name="id"></param>
inline void CasUnlock(const int id)
{
	cas_locker = 0;

	//while (true)
	{

	}
	//while (CAS(&cas_locker, 1, 0));
}

void CasSumWorker(const int th_number, const int id)
{
	const int local_target = summary_target / th_number;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		CasLock(id);
		summary += 2;
		CasUnlock(id);
	}
}

bool g_ready = false;
int g_data = 0;

volatile bool gv_ready = false;
volatile int gv_data = 0;
std::mutex g_mutex{};

void Receiver()
{
	while (false == g_ready);
	std::cout << "I got " << g_data << std::endl;
}

void VolatileReceiver()
{
	while (false == gv_ready);
	std::cout << "I got " << gv_data << std::endl;
}

void ReceiverLocken()
{
	g_mutex.lock();
	while (false == g_ready)
	{
		// 이 안의 구문이 없으면 뮤텍스 조차 while문을 없애는 것을 막지 못한다.
		g_mutex.unlock();
		g_mutex.lock();
	}
	g_mutex.unlock();

	g_mutex.lock();
	std::cout << "I got " << g_data << std::endl;
	g_mutex.unlock();
}

void Sender()
{
	std::cout << "데이터 입력: ";

	std::cin >> g_data;
	g_ready = true;
}

void VolatileSender()
{
	std::cout << "데이터 입력: ";

	int temp{};

	std::cin >> temp;
	gv_data = temp;
	gv_ready = true;
}

void SenderLocken()
{
	std::cout << "데이터 입력: ";

	g_mutex.lock();
	std::cin >> g_data;
	g_mutex.unlock();
	g_ready = true;
}

void PetersonLock(const int myID)
{
	const int other = 1 - myID;
	flag[myID] = true;

	victim = myID;

	// 추가!
	std::atomic_thread_fence(std::memory_order_seq_cst);
	while (flag[other] && victim == myID)
	{
	}
}

void PetersonUnlock(const int myID)
{
	flag[myID] = false;
}

void AtomicPetersonLock(const int myID)
{
	const int other = 1 - myID;
	ga_flags[myID] = true;

	ga_victim = myID;
	while (ga_flags[other] && ga_victim == myID)
	{
	}
}

void AtomicPetersonUnlock(const int myID)
{
	ga_flags[myID] = false;
}

// 맞는 결과
void PetersonSumWorker1(int id)
{
	PetersonLock(id);

	for (int i = 0; i < 2500000; i++)
	{
		// summary = summary + 2;
		summary += 2;
	}

	PetersonUnlock(id);
}

// 틀린 결과
// 게다가 Mutex 보다 느리다!
void PetersonSumWorker2(int id)
{
	for (int i = 0; i < 2500000; i++)
	{
		PetersonLock(id);
		//summary += 2;
		summary = summary + 2;
		PetersonUnlock(id);
	}
}

// 
void PetersonSumWorker3(int th_count, int id)
{
	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	int local_sum = 0;

	for (int i = 0; i < local_times; i++)
	{
		local_sum += 2;
	}

	AtomicPetersonLock(id);
	summary += local_sum;
	AtomicPetersonUnlock(id);
}

// 
void PetersonSumWorker4(int th_count, int id)
{
	const int local_target = summary_target / th_count;
	const int local_times = local_target / 2;

	for (int i = 0; i < local_times; i++)
	{
		AtomicPetersonLock(id);
		summary += 2;
		AtomicPetersonUnlock(id);
	}
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		std::vector<std::jthread> workers{};
		workers.reserve(th_count);

		auto clock_before = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(CasSumWorker, th_count, i);
		}

		for (auto& th : workers)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		auto clock_after = std::chrono::high_resolution_clock::now();

		auto elapsed_time = clock_after - clock_before;
		auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

		std::cout << "스레드 수: " << th_count << "개\n";
		std::cout << "합계: " << summary << "\n";
		std::cout << "시간: " << ms_time << "\n";

		summary = 0;
	}

	return 0;
}