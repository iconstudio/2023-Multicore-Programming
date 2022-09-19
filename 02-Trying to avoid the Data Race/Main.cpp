#include "Main.hpp"

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

int summary = 0;
volatile int victim = 0;
volatile bool flag[2] = { false, false };

void PetersonLock(const int myID)
{
	const int other = 1 - myID;
	flag[myID] = true;

	victim = myID;
	while (flag[other] && victim == myID)
	{
	}
}

void PetersonUnlock(const int myID)
{
	flag[myID] = false;
}

// 맞는 결과
void PetersonSumWorker1(int id)
{
	PetersonLock(id);

	for (int i = 0; i < 25000000; i++)
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
	for (int i = 0; i < 25000000; i++)
	{
		PetersonLock(id);
		// summary = summary + 2;
		summary += 2;
		PetersonUnlock(id);
	}
}

// 
void PetersonSumWorker3(int id)
{
	int local_sum = 0;

	for (int i = 0; i < 25000000; i++)
	{
		// summary = summary + 2;
		local_sum += 2;
	}

	PetersonLock(id);
	summary += local_sum;
	PetersonUnlock(id);
}

int main()
{
	//std::jthread th1{ VolatileSender };
	//std::jthread th2{ VolatileReceiver };

	auto clock_before = std::chrono::high_resolution_clock::now();

	std::jthread th1{ PetersonSumWorker3, 0 };
	std::jthread th2{ PetersonSumWorker3, 1 };

	if (th1.joinable())
	{
		th1.join();
	}

	if (th2.joinable())
	{
		th2.join();
	}

	auto clock_after = std::chrono::high_resolution_clock::now();

	auto elapsed_time = clock_after - clock_before;
	auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

	std::cout << "합계: " << summary << "\n";
	std::cout << "시간: " << ms_time << "\n";

	return 0;
}