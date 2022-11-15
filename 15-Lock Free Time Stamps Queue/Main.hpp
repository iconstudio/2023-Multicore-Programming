#pragma once
#include <windows.h>
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
	int value = -1;
	Node* volatile handle = nullptr;
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
			Node* handle = last->handle;

			// 다른 스레드에서 tail을 수정했다.
			if (last != tail) continue;

			// 내가 갖고 있는 tail이 마지막 노드이다.
			if (nullptr == handle)
			{
				// 아직 tail은 전진을 안 했다.
				// 새로운 노드를 next에 삽입한다.
				if (CAS(&(last->handle), nullptr, node))
				{
					// tail을 전진시킨다.
					CAS(&tail, last, node);
					return;
				}
			}
			else
			{
				CAS(&tail, last, handle);
			}
		}
	}

	int Dequeue()
	{
		while (true)
		{
			Node* first = head;
			Node* last = tail;
			Node* handle = first->handle;

			// 상기한 변수들을 얻어올 때 다른 스레드에서 수정했으면 다시 시작한다.
			if (first != head)
			{
				continue;
			}

			// first가 보초 노드이다.
			if (handle == nullptr)
			{
				//EMPTY_ERROR();
				return -1;
			}

			// 다른 스레드에서 head와 tail을 수정했다.
			if (first == last)
			{
				// tail을 전진시키고 다시 시작한다.
				CAS(&tail, last, handle);
				continue;
			}

			// nullptr을 참조하지 않도록 미리 값을 가져온다.
			const int result = handle->value;

			// head를 전진시킨다.
			if (false == CAS(&head, first, handle))
			{
				continue;
			}

			// 기존의 노드 삭제
			first->handle = nullptr;
			delete first;
			return result;
		}
	}

	void Clear()
	{
		Node* it = head->handle;

		while (it != nullptr)
		{
			Node* handle = it->handle;
			delete it;
			it = handle;
		}

		head->handle = nullptr;
		tail = head;
	}

	void Print()
	{
		Node* it = head->handle;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it->value << ", ";

			it = it->handle;
		}
	}

	Node* volatile head;
	Node* volatile tail;
};

using llong = long long;

struct StampNode;
struct alignas(16) StampPtr
{
	StampNode* handle = nullptr;
	int stamp = 0;

	inline void Set(StampNode* other)
	{
		handle = other;
		stamp++;
	}

	inline StampNode* GetHandle()
	{
		return handle;
	}
};

struct StampNode
{
	int value = -1;
	StampPtr next;

	inline void Set(StampNode* other)
	{
		next.Set(other);
	}
};

class StampedQueue
{
public:
	StampedQueue()
		: head(), tail()
	{
		auto init = new StampNode;

		head.Set(init);
		tail.Set(init);
	}

	bool CAS(StampPtr* handle, StampNode* old_node, StampNode* const new_node, int old_stamp, const int& new_stamp)
	{
		StampPtr old_handle{ old_node, old_stamp };
		//StampPtr new_handle{ new_node, new_stamp };

		return ::_InterlockedCompareExchange128
		(
			reinterpret_cast<LONG64 volatile*>(handle),
			reinterpret_cast<LONG64>(new_node),
			new_stamp,
			reinterpret_cast<LONG64*>(&old_handle)
		);
	}

	void Enqueue(const int& value)
	{
		auto node = new StampNode{ value };

		while (true)
		{
			StampPtr last = tail;
			StampPtr next = last.handle->next;

			// 다른 스레드에서 tail을 수정했다.
			if (last.stamp != tail.stamp) continue;

			// 내가 갖고 있는 tail이 마지막 노드이다.
			if (nullptr == next.handle)
			{
				// 아직 tail은 전진을 안 했다.
				// 새로운 노드를 next에 삽입한다.
				if (CAS(&last.handle->next, nullptr, node, next.stamp, next.stamp + 1))
				{
					// tail을 전진시킨다.
					CAS(&tail, last.handle, node, last.stamp, last.stamp + 1);
					return;
				}
			}
			else
			{
				// tail과 next.handle을 비교해서 ABA 문제 해결
				CAS(&tail, last.handle, next.handle, last.stamp, last.stamp + 1);
			}
		}
	}

	int Dequeue()
	{
		return 0;
		while (true)
		{
			/*
			Node* first = head;
			Node* last = tail;
			Node* handle = first->handle;

			// 상기한 변수들을 얻어올 때 다른 스레드에서 수정했으면 다시 시작한다.
			if (first != head)
			{
				continue;
			}

			// first가 보초 노드이다.
			if (handle == nullptr)
			{
				//EMPTY_ERROR();
				return -1;
			}

			// 다른 스레드에서 head와 tail을 수정했다.
			if (first == last)
			{
				// tail을 전진시키고 다시 시작한다.
				CAS(&tail, last, handle);
				continue;
			}

			// nullptr을 참조하지 않도록 미리 값을 가져온다.
			const int result = handle->value;

			// head를 전진시킨다.
			if (false == CAS(&head, first, handle))
			{
				continue;
			}

			// 기존의 노드 삭제
			first->handle = nullptr;
			delete first;
			return result;
			*/
		}
	}

	void Clear()
	{
		auto it = head.handle;

		while (nullptr != head.GetHandle()->next.GetHandle())
		{
			auto handle = head.GetHandle();

			head.Set(head.GetHandle()->next.GetHandle());

			delete handle;
		}

		head.handle = nullptr;
		tail = head;
	}

	void Print()
	{
		auto it = head.handle->next;

		for (int i = 0; i < 20; i++)
		{
			std::cout << it.handle->value << ", ";

			it = it.handle->next;
		}
	}

	StampPtr head;
	StampPtr tail;
};
