#include <stdlib.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <set>
#include <chrono>

using namespace std;
using namespace chrono;

using PtrType = size_t;
constexpr PtrType MASK = (std::numeric_limits<PtrType>::max() >> 1) << 1; // 0xFFFFFFFFFFFFFFFE
constexpr int MAX_THREADS = 16;
constexpr int NUMBER_TEST = 40000;
constexpr int KEY_RANGE = 1000;
thread_local int thread_id;

using Response = bool;
class Consensus
{
	PtrType result;

	bool CAS(PtrType old_value, const PtrType& new_value)
	{
		return std::atomic_compare_exchange_strong(reinterpret_cast<std::atomic<PtrType>*>(&result), &old_value, new_value);
	}

public:
	Consensus()
	{
		result = -1;
	}

	PtrType decide(const PtrType& value)
	{
		if (CAS(-1, value))
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

class LFNODE
{
public:
	int key;
	PtrType next;

	LFNODE()
	{
		next = 0;
	}

	LFNODE(const int& value)
	{
		key = value;
		next = 0;
	}

	LFNODE* GetNext()
	{
		return reinterpret_cast<LFNODE*>(next & MASK);
	}

	void SetNext(LFNODE* ptr)
	{
		next = reinterpret_cast<PtrType>(ptr);
	}

	LFNODE* GetNextWithMark(bool* mark)
	{
		auto temp = next;
		*mark = (temp % 2) == 1;

		return reinterpret_cast<LFNODE*>(temp & MASK);
	}

	bool CAS(PtrType old_value, const PtrType& new_value)
	{
		return std::atomic_compare_exchange_strong
		(
			reinterpret_cast<std::atomic<PtrType>*>(&next),
			&old_value, new_value
		);
	}

	bool CAS(LFNODE* old_next, LFNODE* new_next, bool old_mark, bool new_mark)
	{
		auto old_value = reinterpret_cast<PtrType>(old_next);
		if (old_mark)
			old_value = old_value | 0x1;
		else
			old_value = old_value & MASK;

		auto new_value = reinterpret_cast<PtrType>(new_next);
		if (new_mark)
			new_value = new_value | 0x1;
		else
			new_value = new_value & MASK;

		return CAS(old_value, new_value);
	}

	bool TryMark(LFNODE* ptr)
	{
		auto old_value = reinterpret_cast<PtrType>(ptr) & MASK;
		auto new_value = old_value | 1;
		return CAS(old_value, new_value);
	}

	bool IsMarked()
	{
		return 0 != (next & 1);
	}
};

class LFSET
{
public:
	LFSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.SetNext(&tail);
	}

	void Find(const int& key, LFNODE** pred, LFNODE** curr)
	{
	retry:
		LFNODE* prev = &head;
		LFNODE* it = prev->GetNext();

		while (true)
		{
			bool removed = false;
			LFNODE* succ = it->GetNextWithMark(&removed);

			while (removed)
			{
				if (false == prev->CAS(it, succ, false, false))
				{
					goto retry;
				}

				it = succ;
				succ = it->GetNextWithMark(&removed);
			}

			if (key <= it->key)
			{
				*pred = prev;
				*curr = it;
				return;
			}

			prev = it;
			it = it->GetNext();
		}
	}

	bool Add(const int& key)
	{
		LFNODE* pred;
		LFNODE* curr;

		while (true)
		{
			Find(key, &pred, &curr);

			if (curr->key == key)
			{
				return false;
			}
			else
			{
				LFNODE* node = new LFNODE(key);
				node->SetNext(curr);

				if (pred->CAS(curr, node, false, false))
				{
					return true;
				}
				else
				{
					delete node;
				}
			}
		}
	}

	bool Remove(const int& key)
	{
		LFNODE* pred;
		LFNODE* curr;

		while (true)
		{
			Find(key, &pred, &curr);

			if (curr->key != key)
			{
				return false;
			}
			else
			{
				auto succ = curr->GetNext();
				if (!curr->TryMark(succ))
				{
					continue;
				}

				pred->CAS(curr, succ, false, false);
				// delete curr;

				return true;
			}
		}
	}

	bool Contains(const int& key)
	{
		LFNODE* curr = &head;
		bool check = false;

		while (curr->key < key)
		{
			curr = curr->GetNextWithMark(&check);
		}

		return !check && key == curr->key;
	}

	Response Apply(const Invocation& invoc)
	{
		switch (invoc.method)
		{
			case M_ADD:
			{
				return Add(invoc.input);
			}
			break;

			case M_REMOVE:
			{
				return Remove(invoc.input);
			}
			break;

			case M_CONTAINS:
			{
				return Contains(invoc.input);
			}
			break;
		}

		return false;
	}

	void Clear()
	{
		while (head.GetNext() != &tail)
		{
			LFNODE* temp = head.GetNext();
			head.next = temp->next;
			delete temp;
		}
	}

	void Print()
	{
		LFNODE* ptr = head.GetNext();
		std::cout << "Result: ";

		for (int i = 0; i < 20; ++i)
		{
			std::cout << ptr->key << ", ";

			if (&tail == ptr) break;
			ptr = ptr->GetNext();
		}

		std::cout << '\n';
	}

	LFNODE head, tail;
};

class UNIVERSAL_NODE
{
public:
	Invocation invoc;
	Consensus decideNext;
	UNIVERSAL_NODE* next;
	volatile int seq;

	UNIVERSAL_NODE()
	{
		invoc = {};
		seq = 0;
		next = nullptr;
	}

	UNIVERSAL_NODE(const Invocation& input_invoc)
	{
		invoc = input_invoc;
		next = nullptr;
		seq = 0;
	}
};

class SeqObject
{
	std::set<int> seq_set;

public:
	SeqObject()
	{}

	~SeqObject()
	{}

	Response Apply(const Invocation& invoc)
	{
		Response res{};

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

	void Clear()
	{
		seq_set.clear();
	}

	void Print()
	{
		std::cout << "First 20 item : ";

		int count = 20;
		for (auto n : seq_set)
		{
			std::cout << n;
			if (count-- == 0) break;
			std::cout << ", ";
		}

		std::cout << std::endl;
	}
};

class LFUniversal
{
	UNIVERSAL_NODE* head[MAX_THREADS];
	UNIVERSAL_NODE* tail;

public:
	LFUniversal()
	{
		tail = new UNIVERSAL_NODE;
		tail->seq = 1;

		for (int i = 0; i < MAX_THREADS; ++i)
		{
			head[i] = tail;
		}
	}

	~LFUniversal()
	{
		Clear();
	}

	Response Apply(const Invocation& invoc)
	{
		UNIVERSAL_NODE* prefer = new UNIVERSAL_NODE{ invoc };

		while (prefer->seq == 0)
		{
			UNIVERSAL_NODE* before = GetMaxNode();
			UNIVERSAL_NODE* after = reinterpret_cast<UNIVERSAL_NODE*>(before->decideNext.decide(reinterpret_cast<PtrType>(prefer)));

			before->next = after;
			after->seq = before->seq + 1;

			head[thread_id] = after;
		}

		// std::set<int>
		SeqObject my_object;

		UNIVERSAL_NODE* curr = tail->next;
		while (curr != prefer)
		{
			my_object.Apply(curr->invoc);
			curr = curr->next;
		}

		return my_object.Apply(curr->invoc);
	}

	UNIVERSAL_NODE* GetMaxNode()
	{
		UNIVERSAL_NODE* max_node = head[0];
		for (int i = 1; i < MAX_THREADS; i++)
		{
			if (max_node->seq < head[i]->seq) max_node = head[i];
		}

		return max_node;
	}

	void Clear()
	{
		while (tail != nullptr)
		{
			UNIVERSAL_NODE* temp = tail;
			tail = tail->next;
			delete temp;
		}

		tail = new UNIVERSAL_NODE;
		tail->seq = 1;
		for (int i = 0; i < MAX_THREADS; ++i)
			head[i] = tail;
	}

	void Print()
	{
		UNIVERSAL_NODE* before = GetMaxNode();
		UNIVERSAL_NODE* curr = tail->next;

		SeqObject my_object;
		while (true)
		{
			my_object.Apply(curr->invoc);
			if (curr == before) break;
			curr = curr->next;

		}

		my_object.Print();
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

	void Clear()
	{
		seq_object.Clear();
	}

	void Print()
	{
		seq_object.Print();
	}
};

class WFUniversal
{
	UNIVERSAL_NODE* head[MAX_THREADS];
	UNIVERSAL_NODE* announce[MAX_THREADS];
	UNIVERSAL_NODE* tail;

public:
	WFUniversal()
	{
		tail = new UNIVERSAL_NODE;
		tail->seq = 1;

		for (int i = 0; i < MAX_THREADS; ++i)
		{
			head[i] = tail;
			announce[i] = tail;
		}
	}

	~WFUniversal()
	{
		Clear();
	}

	Response Apply(const Invocation& invoc)
	{
		const int i = thread_id;
		announce[i] = new UNIVERSAL_NODE{ invoc };
		head[i] = GetMaxNode();

		while (0 == announce[i]->seq)
		{
			UNIVERSAL_NODE* before = head[i];
			UNIVERSAL_NODE* help = announce[(before->seq + 1) % MAX_THREADS];
			UNIVERSAL_NODE* prefer = nullptr;
			if (0 == help->seq)
			{
				prefer = help;
			}
			else
			{
				prefer = announce[i];
			}

			auto after = reinterpret_cast<UNIVERSAL_NODE*>(before->decideNext.decide(reinterpret_cast<PtrType>(prefer)));

			before->next = after;
			after->seq = before->seq + 1;
			head[i] = after;
		}

		SeqObject my_object{};

		auto curr = tail->next;
		while (curr != announce[i])
		{
			my_object.Apply(curr->invoc);
			curr = curr->next;
		}
		head[i] = announce[i];

		return my_object.Apply(curr->invoc);
	}

	UNIVERSAL_NODE* GetMaxNode()
	{
		auto max_node = head[0];
		for (int i = 1; i < MAX_THREADS; i++)
		{
			if (max_node->seq < head[i]->seq) max_node = head[i];
		}

		return max_node;
	}

	void Clear()
	{
		while (tail != nullptr)
		{
			auto temp = tail;
			tail = tail->next;
			delete temp;
		}

		tail = new UNIVERSAL_NODE;
		tail->seq = 1;
		for (int i = 0; i < MAX_THREADS; ++i)
		{
			head[i] = tail;
			announce[i] = tail;
		}
	}

	void Print()
	{
		auto before = GetMaxNode();
		auto curr = tail->next;

		SeqObject my_object;
		while (true)
		{
			my_object.Apply(curr->invoc);
			if (curr == before) break;
			curr = curr->next;

		}

		my_object.Print();
	}
};

//#define UNIVERSAL_TESTER LFSET
//#define UNIVERSAL_TESTER SeqObject
//#define UNIVERSAL_TESTER LFUniversal
//#define UNIVERSAL_TESTER MutexUniversal
#define UNIVERSAL_TESTER WFUniversal

UNIVERSAL_TESTER my_set{};
void Worker(const int& number_threads, const int& id);

int main()
{
	std::vector<std::thread> worker_threads{};

	thread_id = 0;

	for (int num_thread = 1; num_thread <= 8; num_thread *= 2)
	{
		const auto start = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			worker_threads.emplace_back(Worker, num_thread, i + 1);
		}

		for (auto& pth : worker_threads)
		{
			pth.join();
		}

		const auto du = high_resolution_clock::now() - start;
		worker_threads.clear();

		std::cout << num_thread << " Threads - ½Ã°£: ";
		std::cout << duration_cast<milliseconds>(du).count() << "ms \n";
		my_set.Print();
		std::cout << '\n';

		my_set.Clear();
	}

	return 0;
}

void Worker(const int& number_threads, const int& id)
{
	thread_id = id;

	Invocation invoc{};

	for (int i = 0; i < NUMBER_TEST / number_threads; ++i)
	{
		if (rand() % 2 || i < 32)
		{
			my_set.Apply({ M_ADD, i });
		}
		else
		{
			switch (rand() % 3)
			{
				case 0:
				{
					invoc.method = M_ADD;
				}
				break;

				case 1:
				{
					invoc.method = M_REMOVE;
					break;
				}
				case 2:
				{
					invoc.method = M_CONTAINS;
				}
				break;
			}

			invoc.input = rand() % KEY_RANGE;
			my_set.Apply(invoc);
		}
	}
}
