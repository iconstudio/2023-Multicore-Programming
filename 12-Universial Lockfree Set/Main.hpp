#pragma once
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <set>

using namespace std;
using namespace chrono;

constexpr auto NUM_TEST = 4000;
constexpr auto KEY_RANGE = 1000;
constexpr auto MAX_THREAD = 64;

thread_local int thread_id;

class Consensus
{
	int result;
	bool CAS(int old_value, int new_value)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&result), &old_value, new_value);
	}

public:
	Consensus()
	{
		result = -1;
	}

	~Consensus()
	{}

	int decide(int value)
	{
		if (true == CAS(-1, value))
			return value;
		else return result;
	}
};

typedef bool Response;

enum Method
{
	M_ADD, M_REMOVE, M_CONTAINS
};

struct Invocation
{
	Method method;
	int	input;
};

class SeqObject
{
	set <int> seq_set;

public:
	SeqObject()
	{}

	~SeqObject()
	{}

	Response Apply(const Invocation& invoc)
	{
		Response res;

		// 잘 돌아간다
		switch (invoc.method)
		{
			case M_ADD:
			{
				if (seq_set.count(invoc.input) != 0)
				{
					res = false;
				}
				else
				{
					seq_set.insert(invoc.input);
					res = true;
				}
			}
			break;

			case M_REMOVE:
			{
				if (seq_set.count(invoc.input) == 0)
				{
					res = false;
				}
				else
				{
					seq_set.erase(invoc.input);
					res = true;
				}
			}
			break;

			case M_CONTAINS:
			{
				res = (seq_set.count(invoc.input) != 0);
			}
			break;
		}
		return res;
	}

	void Print20()
	{
		cout << "First 20 item : ";

		int count = 20;
		for (auto n : seq_set)
		{
			if (count-- == 0) break;
			cout << n << ", ";
		}

		cout << endl;

	}

	void clear()
	{
		seq_set.clear();
	}
};

class NODE
{
public:
	Invocation invoc;
	Consensus decideNext;
	NODE* next;
	volatile int seq;

	NODE()
	{
		seq = 0;
		next = nullptr;
	}

	~NODE()
	{}

	NODE(const Invocation& input_invoc)
	{
		invoc = input_invoc;
		next = nullptr;
		seq = 0;
	}
};

class LFUniversal
{
	NODE* head[MAX_THREAD];
	NODE* tail;

public:
	LFUniversal()
	{
		tail = new NODE;
		tail->seq = 1;
		for (int i = 0; i < MAX_THREAD; ++i)
			head[i] = tail;
	}

	~LFUniversal()
	{
		clear();
	}

	Response Apply(const Invocation& invoc)
	{
		// 나의 노드
		NODE* prefer = new NODE{ invoc };

		// Log에 추가
		while (prefer->seq == 0)
		{
			NODE* before = GetMaxNODE();
			NODE* after = reinterpret_cast<NODE*>(before->decideNext.decide(reinterpret_cast<int>(prefer)));

			before->next = after;
			after->seq = before->seq + 1;

			// 나의 헤드
			head[thread_id] = after;
		}

		// std::set<int>
		SeqObject my_object;

		// 지금까지 했던 작업을 다시 처음부터 순회한다!
		// 무잠금은 맞으나 매우 비효율적이다. 그래서 아직 mutex가 더 빠르다.
		// 스레드를 무한대로 늘려도 싱글 스레드보다 엄청나게 빨라지진 않는다.
		// +여기서 싱글 스레드의 문제 발생
		{
			NODE* curr = tail->next;
			while (curr != prefer)
			{
				my_object.Apply(curr->invoc);
				curr = curr->next;
			}

			// 제일 마지막 노드 (= 나의 노드)
			return my_object.Apply(curr->invoc);
		}
	}

	NODE* GetMaxNODE()
	{
		NODE* max_node = head[0];
		for (int i = 1; i < MAX_THREAD; i++)
		{
			if (max_node->seq < head[i]->seq) max_node = head[i];
		}
		return max_node;
	}

	void Print20()
	{
		NODE* before = GetMaxNODE();

		SeqObject my_object;

		NODE* curr = tail->next;
		while (true)
		{
			my_object.Apply(curr->invoc);
			if (curr == before) break;
			curr = curr->next;

		}
		my_object.Print20();
	}

	void clear()
	{
		while (tail != nullptr)
		{
			NODE* temp = tail;
			tail = tail->next;
			delete temp;
		}
		tail = new NODE;
		tail->seq = 1;
		for (int i = 0; i < MAX_THREAD; ++i)
			head[i] = tail;
	}
};

class MutexUniversal
{
	SeqObject seq_object;
	mutex m_lock;

public:
	MutexUniversal()
	{}

	~MutexUniversal()
	{}

	Response Apply(const Invocation& invoc)
	{
		m_lock.lock();
		Response res = seq_object.Apply(invoc);
		m_lock.unlock();
		return res;
	}

	void Print20()
	{
		seq_object.Print20();
	}

	void clear()
	{
		seq_object.clear();
	}
};

class WFUniversal
{
private:
	NODE* announce[MAX_THREAD];
	NODE* head[MAX_THREAD];
	NODE tail;

public:
	WFUniversal()
		: announce(), head()
		, tail()
	{
		tail.seq = 1;
		
		for (int i = 0; i < MAX_THREAD; ++i)
		{
			head[i] = &tail;
			announce[i] = &tail;
		}
	}

	Response Apply(Invocation invoc)
	{
		const int i = thread_id;

		announce[i] = new NODE(invoc);
		head[i] = GetMaxNode();

		while (announce[i]->seq == 0)
		{
			NODE* before = head[i];
			NODE* help = announce[((before->seq + 1) % MAX_THREAD)];
			NODE* prefer;

			if (help->seq == 0) prefer = help;
			else prefer = announce[i];

			//NODE* after = before->decideNext.decide(prefer);
			NODE* after = reinterpret_cast<NODE*>(before->decideNext.decide(reinterpret_cast<int>(prefer)));
			
			before->next = after;
			after->seq = before->seq + 1;

			head[i] = after;
		}

		SeqObject myObject;
		NODE* current = tail.next;
		while (current != announce[i])
		{
			myObject.Apply(current->invoc);
			current = current->next;
		}
		head[i] = announce[i];

		return myObject.Apply(current->invoc);
	}

	NODE* GetMaxNode()
	{
		NODE* max_node = head[0];

		for (int i = 1; i < MAX_THREAD; i++)
		{
			if (max_node->seq < head[i]->seq)
			{
				max_node = head[i];
			}
		}

		return max_node;
	}

	void Print20()
	{
		NODE* before = GetMaxNode();

		SeqObject my_object;

		NODE* curr = tail.next;
		while (true)
		{
			my_object.Apply(curr->invoc);
			if (curr == before) break;
			curr = curr->next;

		}
		my_object.Print20();
	}

	void clear()
	{
		/*
		while (tail != nullptr)
		{
			NODE* temp = tail;
			tail = tail->next;
			delete temp;
		}
		tail = new NODE;
		tail->seq = 1;
		for (int i = 0; i < MAX_THREAD; ++i)
			head[i] = tail;
		
		//*/
	}
};

//SeqObject my_set; // 여기서 락이 없어서 그냥 프로그램이 죽는다
//MutexUniversal my_set;
//LFUniversal my_set;
WFUniversal my_set;

void Benchmark(int num_thread, int tid)
{
	thread_id = tid;

	Invocation invoc{};

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3)
		{
			case 0: invoc.method = M_ADD; break;
			case 1: invoc.method = M_REMOVE; break;
			case 2: invoc.method = M_CONTAINS; break;
		}

		invoc.input = rand() % KEY_RANGE;
		my_set.Apply(invoc);
	}
}
