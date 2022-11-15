#pragma once
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>

struct Node
{
	unsigned value = -1;
	Node* volatile next = nullptr; // volatile이 아니여도 된다.
};

class LockfreeStack
{
public:
	LockfreeStack()
		: top(nullptr)
	{}

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

	void Push(const unsigned& value)
	{
		auto node = new Node{ value };

		while (true)
		{
			auto last = top;

			if (last != top) continue;

			node->next = last;
			if (CAS(&top, last, node))
			{
				return;
			}
		}
	}

	int Pop()
	{
		while (true)
		{
			Node* first = top;

			if (first != top)
			{
				continue;
			}

			if (first == nullptr)
			{
				return -2;
			}
			
			const int result = first->value;
			Node* next = first->next;

			if (false == CAS(&top, first, next))
			{
				continue;
			}

			//delete first;
			return result;
		}
	}

	void Clear()
	{
		while (nullptr != top)
		{
			auto handle = top;
			auto next = top->next;

			top = next;

			delete handle;
		}
	}

	void Print()
	{
		auto it = top->next;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it->value << ", ";

			it = it->next;
		}
	}

	Node* volatile top;
};
