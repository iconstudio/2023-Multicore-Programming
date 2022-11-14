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

			// 다른 스레드에서 tail을 수정했다.
			if (last != tail) continue;

			// 내가 갖고 있는 tail이 마지막 노드이다.
			if (nullptr == next)
			{
				// 아직 tail은 전진을 안 했다.
				// 새로운 노드를 next에 삽입한다.
				if (CAS(&(last->next), nullptr, node))
				{
					// tail을 전진시킨다.
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

			// 상기한 변수들을 얻어올 때 다른 스레드에서 수정했으면 다시 시작한다.
			if (first != head)
			{
				continue;
			}

			// first가 보초 노드이다.
			if (next == nullptr)
			{
				//EMPTY_ERROR();
				return -1;
			}

			// 다른 스레드에서 head와 tail을 수정했다.
			if (first == last)
			{
				// tail을 전진시키고 다시 시작한다.
				CAS(&tail, last, next);
				continue;
			}

			// nullptr을 참조하지 않도록 미리 값을 가져온다.
			const int result = next->value;

			// head를 전진시킨다.
			if (false == CAS(&head, first, next))
			{
				continue;
			}

			// 기존의 노드 삭제
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
