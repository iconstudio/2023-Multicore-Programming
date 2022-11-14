#pragma once
#include <iostream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <algorithm>

struct Node
{
	int value = -1;
	Node* volatile next = nullptr;
};

class CoarseGrainedQueue
{
public:
	CoarseGrainedQueue()
		: head(), tail()
	{
		head = tail = new Node{ -1 };
	}

	void Enqueue(const int& value)
	{
		std::lock_guard<std::mutex> lock{ mtx_enq };

		Node* node = new Node{ value, nullptr };
		tail->next = node;
		tail = node;
	}

	int Dequeue()
	{
		std::lock_guard<std::mutex> lock{ mtx_deq };

		if (head->next == nullptr)
		{
			return -1;
		}

		Node* prev_head = head;

		int result = head->next->value;
		head = head->next;

		delete prev_head;

		return result;
	}

	void Clear()
	{
		Node* node = head;

		while (node != nullptr)
		{
			Node* next = node->next;
			delete node;
			node = next;
		}

		head = tail = new Node{ -1 };
	}

	void Print()
	{
		Node* it = head;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it->value << ", ";

			it = it->next;
		}
	}

	Node* head, * tail;
	std::mutex mtx_enq, mtx_deq;
};

class LockfreeQueue
{
public:
	LockfreeQueue()
	{
		head = tail = new Node{ -1 };
	}

	bool CAS(Node* volatile* ptr, Node* old_value, Node* new_value)
	{
		auto old = reinterpret_cast<unsigned long long>(old_value);

		return std::atomic_compare_exchange_strong
		(
			reinterpret_cast<volatile std::atomic_ullong*>(ptr),
			&old,
			reinterpret_cast<unsigned long long>(new_value)
		);
	}

	void Enqueue(const int& value)
	{
		Node* node = new Node{ value };

		while (true)
		{
			Node* last = tail;
			Node* next = last->next;
			if (last != tail) continue;

			if (nullptr == next)
			{
				if (CAS(&(last->next), nullptr, node))
				{
					CAS(&tail, last, node);
					return;
				}
			}
			else
			{
				CAS(&tail, last, next);
			}
		}
	}

	int Dequeue()
	{
		while (true)
		{
			Node* first = head;
			Node* last = tail;
			Node* next = first->next;

			if (first != head)
			{
				continue;
			}

			if (next == nullptr)
			{
				//EMPTY_ERROR();
				return -1;
			}

			if (first == last)
			{
				CAS(&tail, last, next);
				continue;
			}

			const int result = next->value;
			if (false == CAS(&head, first, next))
			{
				continue;
			}
			first->next = nullptr;

			delete first;
			return result;
		}
	}

	void Clear()
	{
		Node* it = head->next;

		while (it != nullptr)
		{
			Node* next = it->next;
			delete it;
			it = next;
		}

		head->next = nullptr;
		tail = head;
	}

	void Print()
	{
		Node* it = head->next;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it->value << ", ";

			it = it->next;
		}
	}

	Node* volatile head;
	Node* volatile tail;
};

using namespace std;
using namespace chrono;

constexpr int target_summary = 1000000;

//CoarseGrainedQueue my_set{};
LockfreeQueue my_set{};

void Worker(int number_threads)
{
	for (int i = 0; i < target_summary / number_threads; ++i)
	{
		if (rand() % 2 || i < 1000 / number_threads)
		{
			my_set.Enqueue(i);
		}
		else
		{
			my_set.Dequeue();
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		std::vector<std::thread> threads;

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Worker, num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.Print();
		cout << '\n' << num_threads << " Threads - Exec Time: " << exec_ms << endl;
		cout << endl;

		my_set.Clear();
	}
}
