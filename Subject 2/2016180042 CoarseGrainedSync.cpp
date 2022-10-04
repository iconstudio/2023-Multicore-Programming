#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

struct LinkedNode
{
	constexpr LinkedNode(int value)
		: myValue(value)
		, myNext()
	{}

	void SetNext(LinkedNode* node)
	{
		myNext = node;
	}

	int myValue;
	LinkedNode* myNext;
};

class VoidLock
{
public:
	void lock()
	{}

	void unlock()
	{}
};

class OrderedSet
{
public:
	OrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
		, myLock()
	{
		myHead->SetNext(myTail);
	}

	bool Add(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			result = true;
		}
		else
		{
			auto new_node = new LinkedNode(key);

			new_node->SetNext(curr);
			prev->SetNext(new_node);

			result = true;
		}

		myLock.unlock();
		return result;
	}

	bool Contains(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			result = true;
		}

		myLock.unlock();
		return result;
	}

	bool Remove(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			prev->SetNext(curr->myNext);

			result = true;
		}

		myLock.unlock();
		return result;
	}

	inline void Clear()
	{
		myHead->SetNext(myTail);
	}

	LinkedNode* myHead;
	LinkedNode* myTail;

	// VoidLock
	std::mutex myLock;
};

OrderedSet my_set{};

constexpr int NUM_TEST = 4000000;
constexpr int KEY_RANGE = 1000;

void Worker(const int threads_number)
{
	int key = 0;
	for (int i = 0; i < NUM_TEST / threads_number; i++)
	{
		switch (rand() % 3)
		{
			case 0:
			{
				key = rand() % KEY_RANGE;
				
				my_set.Add(key);
			}
			break;

			case 1:
			{
				key = rand() % KEY_RANGE;
				
				my_set.Remove(key);
			}
			break;

			case 2:
			{
				key = rand() % KEY_RANGE;

				my_set.Contains(key);
			}
			break;

			default:
			{
				std::cout << "Error\n";
				exit(-1);
			}
			break;
		}
	}
}

int main()
{
	for (int th_count = 1; th_count <= 8; th_count *= 2)
	{
		std::vector<std::thread> workers{};
		workers.reserve(th_count);

		auto clock_before = std::chrono::high_resolution_clock::now();

		std::cout << "스레드 수 (" << th_count << ") 시작!\n";

		for (int i = 0; i < th_count; i++)
		{
			workers.emplace_back(Worker, th_count);
		}

		for (auto& th : workers)
		{
			th.join();
		}

		auto clock_after = std::chrono::high_resolution_clock::now();

		auto elapsed_time = clock_after - clock_before;
		auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);

		std::cout << "시간: " << ms_time.count() << "ms\n";
		std::cout << "결과: { ";

		int index = 0;
		auto it = my_set.myHead->myNext;
		for (int i = 0; i < 20; i++)
		{
			if (it == my_set.myTail) break;

			std::cout << it->myValue << ", ";

			it = it->myNext;
		}

		my_set.Clear();

		std::cout << "}\n\n";
	}

	return 0;
}
