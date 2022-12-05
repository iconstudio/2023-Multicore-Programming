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

class LF_NODE_SK;

using LF_HANDLE = unsigned long long;
class LOCKFREE_PTR
{
public:
	LOCKFREE_PTR()
		: LOCKFREE_PTR(false, nullptr)
	{}

	LOCKFREE_PTR(bool mark, LF_NODE_SK* node_handle)
		: next(0)
	{
		next = reinterpret_cast<LF_HANDLE>(node_handle);

		if (mark) // 마지막 비트가 1
		{
			next |= 1;
		}
	}

	// get_pointer
	LF_NODE_SK* GetNodePtr()
	{
		//return reinterpret_cast<LOCKFREE_NODE*>((next >> 1) << 1);
		return reinterpret_cast<LF_NODE_SK*>(next & 0xFFFFFFFFFFFFFFFE);
	}

	// get_removed
	bool GetRemoved()
	{
		return (next & 1) == 1;
	}

	// get_pointer_mark
	LF_NODE_SK* GetRemovedPointer(bool& result)
	{
		LF_HANDLE current_next = next;
		result = (current_next & 1) == 1;

		// 원자적으로 포인터를 반환한다.
		// 이유: 윗줄에서 마킹을 했는데도 정작 next 필드에 적용이 안되었을 수도 있다.
		return reinterpret_cast<LF_NODE_SK*>(current_next & 0xFFFFFFFFFFFFFFFE);
	}

	// CAS
	bool CAS(LF_NODE_SK* old_ptr, LF_NODE_SK* new_ptr, bool old_mark, bool new_mark)
	{
		// 비교 값
		LF_HANDLE old_next = reinterpret_cast<LF_HANDLE>(old_ptr);
		if (old_mark)
		{
			old_next++;
		}

		// 바꿀 값
		LF_HANDLE new_next = reinterpret_cast<LF_HANDLE>(new_ptr);
		if (new_mark)
		{
			new_next++;
		}

		// &next를 형변환
		return std::atomic_compare_exchange_strong
		(
			reinterpret_cast<std::atomic_uint64_t*>(&next)
			, &old_next, new_next
		);
	}

	LF_HANDLE next;
};

class LF_NODE_SK
{
public:
	LF_NODE_SK(const int& v, const int& top)
		: value(v), top_level(top)
		, nexts()
	{}

	LF_NODE_SK() : LF_NODE_SK(-1, 0) {}

	int value;
	int top_level;

	LOCKFREE_PTR nexts[MAX_LEVEL + 1];
};

class LockfreeSkipList
{
	LF_NODE_SK head;
	LF_NODE_SK tail;

public:
	LockfreeSkipList()
		: head(), tail()
	{
		head.value = 0x80000000;
		tail.value = 0x7FFFFFFF;

		tail.top_level = head.top_level = MAX_LEVEL;

		for (auto& next : head.nexts)
		{
			next = LOCKFREE_PTR{ false, &tail };
		}

		//head.next = LOCKFREE_PTR{ false, &tail };
	}

	bool FIND(LF_NODE_SK* (prev)[], LF_NODE_SK* (curr)[], const int& key)
	{
	Restart:
		prev[MAX_LEVEL] = &head;

		for (int i = MAX_LEVEL; 0 <= i; i--)
		{
			curr[i] = prev[i]->nexts[i].GetNodePtr();

			while (true)
			{
				bool removed{};
				LF_NODE_SK* successor = curr[i]->nexts[i].GetRemovedPointer(removed);

				// 마킹이 되어 있지 않은 노드가 나올 때까지 전진
				while (removed)
				{
					if (!prev[i]->nexts[i].CAS
					(
						curr[i], successor,
						false, false
						))
					{
						// 다른 스레드와 충돌! (다른 스레드가 마킹함)
						goto Restart;
					}

					// 현재 i층의 전진 성공!
					curr[i] = successor;

					successor = curr[i]->nexts[i].GetRemovedPointer(removed);
				}

				// 적합한 노드를 찾을 때 까지 전진한다.
				if (curr[i]->value < key)
				{
					prev[i] = curr[i];
					curr[i] = successor;
				}
				else
				{
					if (0 == i)
					{
						return curr[i]->value == key;
					}
					// 

					break;
				}
			}

			/* 위에서 전진한다.
			// 적합한 노드를 찾을 때 까지 전진한다.
			while (curr[i]->value < key)
			{
				prev[i] = curr[i];

				// 위에서 전진 처리를 함으로써 다음 노드 succ을 찾았다.
				//curr[i] = curr[i]->nexts[i].GetNodePtr();
				curr[i] = successor;
			}
			*/
		}
	}

	bool ADD(const int& key)
	{
		LF_NODE_SK* prev[MAX_LEVEL + 1]{};
		LF_NODE_SK* curr[MAX_LEVEL + 1]{};

		while (true)
		{
			bool exists = FIND(prev, curr, key);
			if (!exists)
			{
				return false;
			}

			int new_level = 0;
			for (int i = 0; i < MAX_LEVEL; ++i)
			{
				if ((rand() % 2) == 0) break;
				new_level++;
			}

			// 새로운 노드 생성
			auto node = new LF_NODE_SK{ key, new_level };

			// 새로운 노드를 준비
			for (int i = 0; i <= new_level; i++)
			{
				node->nexts[i] = LOCKFREE_PTR{ false, curr[i] };
			}

			// 삽입 시도
			if (!prev[0]->nexts[0].CAS(curr[0], node, false, false))
			{
				// 지금 아무것도 안 바꿨으므로 문제 없다.
				continue;
			}

			for (int i = 1; i <= new_level; i++)
			{
				while (false == prev[i]->nexts[i].CAS(curr[i], node, false, false))
				{
					FIND(prev, curr, key);
				}
			}

			return true;
		}
	}

	bool REMOVE(int key)
	{
		LF_NODE_SK* prev[MAX_LEVEL + 1]{};
		LF_NODE_SK* curr[MAX_LEVEL + 1]{};

		while (true)
		{
			bool exists = FIND(prev, curr, key);
			if (!exists)
			{
				return false;
			}

			// 삭제는 위층부터 진행한다.
			int top_level = curr[0]->top_level;

			bool is_failed = false;

			// 바닥 층은 아직 건드리면 안된다.
			for (int i = top_level; 0 < i; i--)
			{
				if (!prev[i]->nexts[i].CAS(curr[i], curr[i], false, true))
				{
					// 다른 스레드에서 FIND, REMOVE 중이다.
					is_failed = true;

					break;
				}
			}

			if (is_failed)
			{
				// 다시 시작
				continue;
			}

			// 위층을 삭제한 부분을 복구하지 않아도 된다.
			// 0 윗층은 단순히 지름길이다.
			if (!prev[0]->nexts[0].CAS(curr[0], curr[0], false, true))
			{
				// 다른 스레드에서 이미 삭제했다.
				continue;
			}

			// 마킹된 노드를 정리한다.
			FIND(prev, curr, key);
			return true;
		}
	}

	bool CONTAINS(const int& key)
	{
		LF_NODE_SK* prev[MAX_LEVEL + 1]{};
		LF_NODE_SK* curr[MAX_LEVEL + 1]{};

		bool exists = FIND(prev, curr, key);

		return exists;
	}

	void PRINT()
	{
		auto it = head.nexts[0].GetNodePtr();

		for (int i = 0; i < 20; ++i)
		{
			if (it == &tail) break;

			std::cout << it->value << ", ";

			it = it->nexts[0].GetNodePtr();
		}

		std::cout << std::endl;
	}

	void CLEAR()
	{
		auto it = head.nexts[0].GetNodePtr();

		while (it != &tail)
		{
			auto t = it;

			it = it->nexts[0].GetNodePtr();

			delete t;
		}

		for (auto& next : head.nexts)
		{
			next = LOCKFREE_PTR{ false, &tail };
		}
	}
};
