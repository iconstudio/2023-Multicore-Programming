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

class LF_NODE_SK
{
public:
	LF_NODE_SK(const int& v, const int& top)
		: value(v), top_level(top)
		, nexts()
		, isRemoved(false), fullyLinked(false)
	{
		for (auto i = 0; i <= MAX_LEVEL; ++i)
		{
			nexts[i] = nullptr;
		}
	}

	LF_NODE_SK() : LF_NODE_SK(-1, 0) {}

	int value;
	int top_level;

	LF_NODE_SK* volatile nexts[MAX_LEVEL + 1];

	volatile bool isRemoved;
	volatile bool fullyLinked;
};

class NODE_SK
{
public:
	NODE_SK(const int& v, const int& top)
		: value(v), top_level(top)
		, nexts()
		, isRemoved(false), fullyLinked(false)
	{
		for (auto i = 0; i <= MAX_LEVEL; ++i)
		{
			nexts[i] = nullptr;
		}
	}

	NODE_SK() : NODE_SK(-1, 0) {}

	void lock()
	{
		myMutex.lock();
	}

	void unlock()
	{
		myMutex.unlock();
	}

	int value;
	int top_level;

	std::recursive_mutex myMutex;
	NODE_SK* volatile nexts[MAX_LEVEL + 1];

	volatile bool isRemoved;
	volatile bool fullyLinked;
};

class SkipList
{
public:
	SkipList()
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

	int Find(int x, NODE_SK* pred[], NODE_SK* curr[])
	{
		int found_level = -1;

		pred[MAX_LEVEL] = &head;

		for (int i = MAX_LEVEL; i >= 0; --i)
		{
			curr[i] = pred[i]->nexts[i];

			// ��� ����
			while (curr[i]->value < x)
			{
				pred[i] = curr[i];
				curr[i] = curr[i]->nexts[i];
			}

			if (-1 == found_level && curr[i]->value == x)
			{
				found_level = i;
			}

			if (i == 0) break;
			pred[i - 1] = pred[i];
		}

		return found_level;
	}

	bool ADD(int x)
	{
		// ��� ������ ���� ������
		NODE_SK* pred[MAX_LEVEL + 1];
		NODE_SK* curr[MAX_LEVEL + 1];

		//std::unique_lock<std::recursive_mutex> guard{ myLocker };

		while (true)
		{
			int found_level = Find(x, pred, curr);
			if (-1 != found_level)
			{
				// ������ ���� Add�� �����ϹǷ� �ٽ� ����
				if (curr[found_level]->isRemoved) continue;

				// ���� ����� �� ���� ���
				while (!curr[found_level]->fullyLinked);
				return false;
			}

			int valid_level = 0;
			for (int i = 0; i < MAX_LEVEL; ++i)
			{
				if ((rand() % 2) == 0) break;
				valid_level++;
			}

			// 
			bool valid = false;

			// �ʿ���� ����� ���� ���� ��� �ϳ��� ��׸鼭 ����
			int last_top_locked_level = 0;

			// ��� ��带 ��װ� ���Ἲ �˻�
			for (int i = 0; i <= valid_level; i++)
			{
				pred[i]->lock();
				last_top_locked_level = i;

				valid = !pred[i]->isRemoved
					&& !curr[i]->isRemoved
					&& pred[i]->nexts[i] == curr[i];

				if (!valid)
				{
					break;
				}
			}
			if (!valid)
			{
				// recursive_lock�� ��� Ƚ����ŭ �ٽ� ��������� �Ѵ�.
				for (int k = 0; k < last_top_locked_level; k++)
				{
					pred[k]->unlock();
				}

				continue;
			}

			// ��� ���� �κ�
			auto newbie = new NODE_SK{ x, valid_level };
			for (int i = 0; i <= valid_level; i++)
			{
				newbie->nexts[i] = curr[i];
			}
			for (int i = 0; i <= valid_level; i++)
			{
				pred[i]->nexts[i] = newbie;
			}

			for (int k = 0; k < last_top_locked_level; k++)
			{
				pred[k]->unlock();
			}

			return true;
		}
	}

	bool REMOVE(const int& x)
	{
		// ��� ������ ���� ������
		NODE_SK* pred[MAX_LEVEL + 1]{};
		NODE_SK* curr[MAX_LEVEL + 1]{};

		//std::unique_lock<std::recursive_mutex> guard{ myLocker };
		Find(x, pred, curr);

		if (curr[0]->value != x)
		{
			return false;
		}
		else
		{
			int max_level = curr[0]->top_level;

			for (int i = 0; i <= max_level; i++)
			{
				pred[i]->nexts[i] = curr[i]->nexts[i];
			}

			// curr[0~i]�� ����
			auto& target = curr[0];


			return true;
		}

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
	std::recursive_mutex myLocker;
};
