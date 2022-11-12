#pragma once
#include <iostream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>

struct Node
{
	int value = -1;
	Node* volatile next = nullptr;
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
