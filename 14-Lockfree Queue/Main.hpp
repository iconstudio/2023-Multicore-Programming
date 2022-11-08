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
		: head(), tail()
	{
		head = tail = new Node{ -1 };
	}

	bool CAS(Node* volatile* ptr, Node* old_value, Node* new_value)
	{
		return std::atomic_compare_exchange_strong
		(
			reinterpret_cast<volatile std::atomic_llong*>(ptr),
			reinterpret_cast<long long*>(old_value),
			reinterpret_cast<long long>(new_value)
		);
	}

	void Enqueue(int value)
	{
		Node* node = new Node{ value, nullptr };
		
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
			if (first->next == nullptr)
			{
				//EMPTY_ERROR();
			}
			
			if (!CAS(&head, first, first->next))
			{
				continue;
			}
			
			const int result = first->next->value;
			delete first;
			
			return result;
		}\
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

	Node* volatile head;
	Node* volatile tail;
};
