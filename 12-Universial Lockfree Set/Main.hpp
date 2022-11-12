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

constexpr auto MAX_THREAD = 64;
thread_local int thread_id;

typedef bool Response;

class Consensus
{
	ptrdiff_t result;
	
	bool CAS(ptrdiff_t old_value, const ptrdiff_t& new_value)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_ptrdiff_t*>(&result), &old_value, new_value);
	}

public:
	Consensus()
	{
		result = -1;
	}

	~Consensus()
	{}

	ptrdiff_t decide(const ptrdiff_t& value)
	{
		if (true == CAS(-1, value))
			return value;
		else
			return result;
	}
};

enum Method
{
	M_ADD, M_REMOVE, M_CONTAINS
};

struct Invocation
{
	Method method;
	int	input;
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

		// �� ���ư���
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
		// ���� ���
		NODE* prefer = new NODE{ invoc };

		// Log�� �߰�
		while (prefer->seq == 0)
		{
			NODE* before = GetMaxNODE();
			NODE* after = reinterpret_cast<NODE*>(before->decideNext.decide(reinterpret_cast<int>(prefer)));

			before->next = after;
			after->seq = before->seq + 1;

			// ���� ���
			head[thread_id] = after;
		}

		// std::set<int>
		SeqObject my_object;

		// ���ݱ��� �ߴ� �۾��� �ٽ� ó������ ��ȸ�Ѵ�!
		// ������� ������ �ſ� ��ȿ�����̴�. �׷��� ���� mutex�� �� ������.
		// �����带 ���Ѵ�� �÷��� �̱� �����庸�� ��û���� �������� �ʴ´�.
		// +���⼭ �̱� �������� ���� �߻�
		{
			NODE* curr = tail->next;
			while (curr != prefer)
			{
				my_object.Apply(curr->invoc);
				curr = curr->next;
			}

			// ���� ������ ��� (= ���� ���)
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

			if (help->seq == 0)
				prefer = help;
			else
				prefer = announce[i];

			//NODE* after = before->decideNext.decide(prefer);
			NODE* after = reinterpret_cast<NODE*>(before->decideNext.decide(reinterpret_cast<ptrdiff_t>(prefer)));

			before->next = after;
			after->seq = before->seq + 1;

			head[i] = after;
		}

		SeqObject myObject{};

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
