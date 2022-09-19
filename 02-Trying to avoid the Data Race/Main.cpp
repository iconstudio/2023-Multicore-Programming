#include "Main.hpp"

int g_data = 0;
bool g_ready = false;

void Receiver()
{
	while (false == g_ready);

	std::cout << "I got " << g_data << std::endl;
}

void Sender()
{
	std::cout << "데이터 입력: ";

	std::cin >> g_data;
	g_ready = true;
}

int main()
{
	std::jthread th1{ Sender };
	std::jthread th2{ Receiver };

	return 0;
}