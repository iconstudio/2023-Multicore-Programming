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
	Node* volatile next = nullptr;
};

class Stack
{
public:
	Stack()
		: top(nullptr)
	{}
	
	void Push(const unsigned& value)
	{
		auto node = new Node{ value };
		
		std::lock_guard<std::mutex> guard{ mutex };

		node->next = top;
		top = node;
	}

	int Pop()
	{
		std::lock_guard<std::mutex> guard{ mutex };
		
		if (nullptr == top)
		{
			return -2;
		}
		
		int result = top->value;
		Node* old_top = top;
		top = top->next;
		
		delete old_top;
		return result;
	}

	void Clear()
	{
		while (nullptr != top)
		{
			auto handle = top->next;
			top = handle->next;

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
	
	Node* top;
	std::mutex mutex;
};
