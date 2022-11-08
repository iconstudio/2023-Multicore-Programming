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
		Node* node = head->next;
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
		Node* it = head->next;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it->value << ", ";

			it = it->next;
		}
	}

	Node* head, * tail;
	std::mutex mtx_enq, mtx_deq;
};
