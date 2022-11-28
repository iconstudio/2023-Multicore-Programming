#pragma once
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <algorithm>

constexpr int MAX_LEVEL = 100;
constexpr int TARGET_SUM = 40000000;

class NODE_SK
{
public:
	NODE_SK(const int& v, const int& top)
		: value(v), top_level(top)
		, nexts()
	{
		for (auto i = 0; i <= MAX_LEVEL; ++i)
		{
			nexts[i] = nullptr;
		}
	}

	NODE_SK() : NODE_SK(-1, 0) {}

	int value;
	int top_level;
	NODE_SK* volatile nexts[MAX_LEVEL + 1];
};

class ProceduralSkipList
{
public:
	ProceduralSkipList()
		: head()
	{
		head.value = 0x80000000;
		head.value = 0x7FFFFFFF;
		tail.top_level = head.top_level = MAX_LEVEL;

		for (NODE_SK* volatile& node : head.nexts)
		{
			node = &tail;
		}
	}

	void Find(int x, NODE_SK* pred[], NODE_SK* curr[])
	{
		// 먼저 맨 위와 맨 아래의 노드를 검사
		pred[MAX_LEVEL] = &head;

		// 나머지 head 밑의 노드들을 검사
		for (int i = MAX_LEVEL; i >= 0; --i)
		{
			// 다음
			// currs[MAX_LEVEL] = preds[MAX_LEVEL]->nexts[MAX_LEVEL]
			curr[i] = pred[i]->nexts[i];

			// 노드 전진
			while (curr[i]->value < x)
			{
				pred[i] = curr[i];
				curr[i] = curr[i]->nexts[i];
			}

			// 지금 찾은 것을 기준으로 루프 시작
			if (i == 0) break;
			pred[i - 1] = pred[i];
		}
	}

	bool ADD(int x)
	{
		// 모든 계층에 대한 포인터
		NODE_SK* pred[MAX_LEVEL + 1];
		NODE_SK* curr[MAX_LEVEL + 1];

		std::unique_lock<std::mutex> guard{ myLocker };

		Find(x, pred, curr);
		if (curr[0]->value == x)
		{
			return false;
		}
		else
		{
			// new_level이 높아질 때마다 현존하는 노드의 절반이 new_level보다 큼을 의미한다.
			int new_level = 0;

			for (int i = 0; i < MAX_LEVEL; ++i)
			{
				if ((rand() % 2) == 0) break;
				new_level++;
			}

			NODE_SK* node = new NODE_SK{ x, new_level };
			for (int i = 0; i <= new_level; ++i)
				node->nexts[i] = curr[i];

			for (int i = 0; i <= new_level; ++i)
				pred[i]->nexts[i] = node;

			return true;
		}
	}

	bool REMOVE(const int& x)
	{
		// 모든 계층에 대한 포인터
		NODE_SK* pred[MAX_LEVEL + 1]{};
		NODE_SK* curr[MAX_LEVEL + 1]{};

		std::unique_lock<std::mutex> guard{ myLocker };
		//Find(x, pred, curr);

		return false;
	}

	bool CONTAINS(const int& value)
	{
		auto curr = &head;

		while (curr->value < value)
		{
			curr = curr->nexts[0];
		}

		return curr->value == value;
	}

	void PRINT()
	{
		NODE_SK* ptr = head.nexts[0];

		for (int i = 0; i < 20; ++i)
		{
			if (ptr == &tail) break;
			std::cout << ptr->value << ", ";

			ptr = ptr->nexts[0];
		}

		std::cout << std::endl;
	}

	void CLEAR()
	{
		auto* ptr = head.nexts[0];
		while (ptr != &tail)
		{
			auto* handle = ptr;
			ptr = ptr->nexts[0];
			delete ptr;
		}
		for (auto& node : head.nexts)
		{
			node = &tail;
		}
	}

	NODE_SK head, tail;
	std::mutex myLocker;
};
