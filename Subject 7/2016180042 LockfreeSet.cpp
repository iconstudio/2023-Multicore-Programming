#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>

using namespace std;
using namespace chrono;

class null_mutex
{
public:
	void lock()
	{}

	void unlock()
	{}
};

class NODE
{
	mutex n_lock;
public:
	int v;
	NODE* volatile next;

	NODE() : v(-1), next(nullptr)
	{}

	NODE(int x) : v(x), next(nullptr)
	{}

	void lock()
	{
		n_lock.lock();
	}

	void unlock()
	{
		n_lock.unlock();
	}
};

class MARKED_NODE
{
public:
	int v;
	MARKED_NODE* volatile next;
	volatile bool removed;

	MARKED_NODE()
		: MARKED_NODE(-1)
	{}

	MARKED_NODE(int x)
		: v(x)
		, next(nullptr)
		, removed(false)
	{}

	void lock()
	{
		n_lock.lock();
	}

	void unlock()
	{
		n_lock.unlock();
	}

	void mark()
	{
		removed = true;
	}

private:
	mutex n_lock;
};

class NODE_SP
{
	mutex n_lock;

public:
	int v;
	shared_ptr <NODE_SP> next;
	volatile bool removed;

	NODE_SP() : v(-1), next(nullptr), removed(false)
	{}

	NODE_SP(int x) : v(x), next(nullptr), removed(false)
	{}

	void lock()
	{
		n_lock.lock();
	}

	void unlock()
	{
		n_lock.unlock();
	}
};

class SET
{
	NODE head, tail;
	mutex ll;
public:
	SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	bool ADD(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x)
		{
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			ll.unlock();
			return true;
		}
		else
		{
			ll.unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x)
		{
			ll.unlock();
			return false;
		}
		else
		{
			prev->next = curr->next;
			delete curr;
			ll.unlock();
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		NODE* prev = &head;

		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		bool res = (curr->v == x);
		ll.unlock();

		return res;
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class F_SET
{
	NODE head, tail;
public:
	F_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	bool ADD(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x)
		{
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			curr->unlock();
			prev->unlock();
			return true;
		}
		else
		{
			curr->unlock();
			prev->unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x)
		{
			curr->unlock();
			prev->unlock();
			return false;
		}
		else
		{
			prev->next = curr->next;
			curr->unlock();
			prev->unlock();
			delete curr;
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;

		curr->lock();
		while (curr->v < x)
		{
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}

		bool res = (curr->v == x);
		curr->unlock();
		prev->unlock();

		return res;
	}

	void print20()
	{
		NODE* p = head.next;

		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}

		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;

		while (p != &tail)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}

		head.next = &tail;
	}
};

class O_SET
{
	NODE head, tail;
public:
	O_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	bool Validate(const NODE* prev, const NODE* curr)
	{
		NODE* it = &head;

		while (it->v <= prev->v)
		{
			if (it == prev)
			{
				return (prev->next == curr);
			}

			it = it->next;
		}

		return false;
	}

	bool ADD(int key)
	{
		while (true)
		{
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < key)
			{
				prev = curr;
				curr = curr->next;
			}

			prev->lock();
			curr->lock();

			if (Validate(prev, curr))
			{
				if (curr->v != key)
				{
					NODE* node = new NODE{ key };
					node->next = curr;
					prev->next = node;

					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}

			curr->unlock();
			prev->unlock();
		}
	}

	bool REMOVE(int key)
	{
		while (true)
		{
			NODE* prev = &head;
			NODE* curr = prev->next;

			while (curr->v < key)
			{
				prev = curr;
				curr = curr->next;
			}

			prev->lock();
			curr->lock();

			if (Validate(prev, curr))
			{
				if (curr->v != key)
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					prev->next = curr->next;

					curr->unlock();
					prev->unlock();

					//delete curr;
					return true;
				}
			}

			curr->unlock();
			prev->unlock();
		}
	}

	bool CONTAINS(int key)
	{
		while (true)
		{
			NODE* prev = &head;
			NODE* curr = prev->next;

			while (curr->v < key)
			{
				prev = curr;
				curr = curr->next;
			}

			prev->lock();
			curr->lock();

			if (Validate(prev, curr))
			{
				bool result = false;

				curr->unlock();
				prev->unlock();

				return (curr->v == key);
			}

			curr->unlock();
			prev->unlock();
		}
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class LAZY_SET
{
	MARKED_NODE head, tail;

public:
	LAZY_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	bool Validate(const MARKED_NODE* prev, const MARKED_NODE* curr)
	{
		return !prev->removed && !curr->removed && prev->next == curr;
	}

	bool ADD(int key)
	{
		while (true)
		{
			auto prev = &head;
			auto curr = prev->next;
			while (curr->v < key)
			{
				prev = curr;
				curr = curr->next;
			}

			prev->lock();
			curr->lock();

			if (Validate(prev, curr))
			{
				if (curr->v != key)
				{
					auto node = new MARKED_NODE{ key };
					node->next = curr;
					prev->next = node;

					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}

			curr->unlock();
			prev->unlock();
		}
	}

	bool REMOVE(int key)
	{
		while (true)
		{
			auto prev = &head;
			auto curr = prev->next;

			while (curr->v < key)
			{
				prev = curr;
				curr = curr->next;
			}

			prev->lock();
			curr->lock();

			if (Validate(prev, curr))
			{
				if (curr->v != key)
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->mark();
					prev->next = curr->next;

					curr->unlock();
					prev->unlock();

					//delete curr;
					return true;
				}
			}

			curr->unlock();
			prev->unlock();
		}
	}

	bool CONTAINS(int key)
	{
		auto curr = &head;

		while (curr->v < key)
		{
			curr = curr->next;
		}

		return curr->v == key && !curr->removed;
	}

	void print20()
	{
		auto it = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (it == &tail) break;

			cout << it->v << ", ";
			it = it->next;
		}
		cout << endl;
	}

	void clear()
	{
		auto it = head.next;

		while (it != &tail)
		{
			auto t = it;
			it = it->next;

			delete t;
		}
		head.next = &tail;
	}
};

class L_SET_SP
{
	shared_ptr <NODE_SP> head, tail;

public:
	L_SET_SP()
	{
		head = make_shared<NODE_SP>(0x80000000);
		tail = make_shared<NODE_SP>(0x7FFFFFFF);
		head->next = tail;
	}

	bool validate(shared_ptr<NODE_SP>& prev, shared_ptr<NODE_SP>& curr)
	{
		return (prev->removed == false) && (curr->removed == false)
			&& (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true)
		{
			shared_ptr<NODE_SP> prev = head;
			shared_ptr<NODE_SP> curr = prev->next;
			while (curr->v < x)
			{
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr))
			{
				if (curr->v != x)
				{
					shared_ptr<NODE_SP> node = make_shared<NODE_SP>(x);;
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else
			{
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true)
		{
			shared_ptr<NODE_SP> prev = head;
			shared_ptr<NODE_SP> curr = prev->next;
			while (curr->v < x)
			{
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr))
			{
				if (curr->v != x)
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else
			{
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<NODE_SP> node = head->next;
		while (node->v < x) node = node->next;
		return ((node->v == x) && (false == node->removed));
	}

	void print20()
	{
		shared_ptr<NODE_SP> p = head->next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};

class LF_NODE;
class LF_PTR
{
	unsigned long long next;

public:
	LF_PTR() : next(0)
	{}

	LF_PTR(bool marking, LF_NODE* ptr)
	{
		next = reinterpret_cast<unsigned long long>(ptr);
		if (true == marking) next = next | 1;
	}

	LF_NODE* get_ptr()
	{
		return reinterpret_cast<LF_NODE*>(next & 0xFFFFFFFFFFFFFFFE);
	}

	bool get_removed()
	{
		return (next & 1) == 1;
	}

	bool attempt_mark(LF_NODE* new_next, const bool mark_value)
	{
		bool old_mark = get_removed();
		auto old_next = get_ptr();

		bool new_mark = new_next->next.get_removed();
		auto new_succ = new_next->next.get_ptr();

		return CAS(old_next, new_succ, old_mark, new_mark);
	}

	LF_NODE* get_ptr_mark(bool* removed)
	{
		unsigned long long curr_next = next;
		*removed = (curr_next & 1) == 1;

		return reinterpret_cast<LF_NODE*>(curr_next & 0xFFFFFFFFFFFFFFFE);
	}

	bool CAS(LF_NODE* old_ptr, LF_NODE* new_ptr, bool o_mark, bool n_mark)
	{
		unsigned long long o_next = reinterpret_cast<unsigned long long>(old_ptr);
		if (true == o_mark) o_next++;

		unsigned long long n_next = reinterpret_cast<unsigned long long>(new_ptr);
		if (true == n_mark) n_next++;

		return atomic_compare_exchange_strong
		(
			reinterpret_cast<atomic_uint64_t*>(&next),
			&o_next, n_next
		);
	}
};

class LF_NODE
{
public:
	int v;
	LF_PTR next;

	LF_NODE() : v(-1), next(false, nullptr)
	{}

	LF_NODE(int x) : v(x), next(false, nullptr)
	{}

	LF_NODE(int x, LF_NODE* ptr) : v(x), next(false, ptr)
	{}
};

class LF_SET
{
	LF_NODE head, tail;

public:
	LF_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = LF_PTR{ false, &tail };
	}

	void Find(LF_NODE*& prev, LF_NODE*& curr, int key)
	{
		while (true)
		{
		retry:
			prev = &head;
			curr = prev->next.get_ptr();

			while (true)
			{
				bool removed;
				LF_NODE* succ = curr->next.get_ptr_mark(&removed);

				while (removed)
				{
					if (false == prev->next.CAS(curr, succ, false, false))
					{
						goto retry;
					}

					curr = succ;
					succ = curr->next.get_ptr_mark(&removed);
				}

				if (curr->v >= key)
				{
					return;
				}

				prev = curr;
				curr = succ;
			}
		}
	}

	bool ADD(int key)
	{
		while (true)
		{
			LF_NODE* prev;
			LF_NODE* curr;
			Find(prev, curr, key);

			if (curr->v != key)
			{
				LF_NODE* node = new LF_NODE{ key, curr };

				if (true == prev->next.CAS(curr, node, false, false))
				{
					return true;
				}

				delete node;
			}
			else
			{
				return false;
			}
		}
	}

	bool REMOVE(int key)
	{
		bool snip = false;
		LF_NODE* prev;
		LF_NODE* curr;

		while (true)
		{
			Find(prev, curr, key);

			if (curr->v != key)
			{
				return false;
			}

			auto successor = curr->next.get_ptr();

			snip = curr->next.attempt_mark(successor, true);
			if (!snip)
			{
				continue;
			}

			prev->next.CAS(curr, successor, false, false);
			return true;
		}

		return true;
	}

	bool CONTAINS(int key)
	{
		auto it = head.next.get_ptr();

		bool marked = false;
		while (it->v < key)
		{
			it = it->next.get_ptr();
			it->next.get_ptr_mark(&marked);
		}

		return (it->v == key && !marked);
	}

	void print20()
	{
		auto it = head.next.get_ptr();

		for (int i = 0; i < 20; i++)
		{
			if (it == &tail) break;
			cout << it->v << ", ";

			it = it->next.get_ptr();
		}

		cout << endl;
	}

	void clear()
	{
		auto it = head.next.get_ptr();

		while (it != &tail)
		{
			auto t = it;
			it = it->next.get_ptr();

			delete t;
		}

		head.next = LF_PTR{ false, &tail };
	}
};

//SET my_set;   // 성긴 동기화
//F_SET my_set;   // 세밀한 동기화
//O_SET my_set; // 낙천적 동기화
//LAZY_SET my_set; // 게으른 동기화
//L_SET_SP my_set; // 게으른 동기화 shared_ptr 구현

LF_SET my_set{};
constexpr int TIMES_TARGET = 4000000;
constexpr int NUMBER_LIMIT = 1000;

class HISTORY
{
public:
	int op;
	int i_value; // input
	bool o_value; // output

	HISTORY(int o, int i, bool re)
		: op(o), i_value(i), o_value(re)
	{}
};

void worker(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < TIMES_TARGET / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % NUMBER_LIMIT;
				my_set.ADD(v);
				break;
			}

			case 1:
			{
				int v = rand() % NUMBER_LIMIT;
				my_set.REMOVE(v);
				break;
			}

			case 2:
			{
				int v = rand() % NUMBER_LIMIT;
				my_set.CONTAINS(v);
				break;
			}
		}
	}
}

void worker_check(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < TIMES_TARGET / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % NUMBER_LIMIT;
				history->emplace_back(0, v, my_set.ADD(v));
				break;
			}

			case 1:
			{
				int v = rand() % NUMBER_LIMIT;
				history->emplace_back(1, v, my_set.REMOVE(v));
				break;
			}

			case 2:
			{
				int v = rand() % NUMBER_LIMIT;
				history->emplace_back(2, v, my_set.CONTAINS(v));
				break;
			}
		}
	}
}

void check_history(array<vector<HISTORY>, 16>& history, int num_threads)
{
	array<int, NUMBER_LIMIT> survive{};

	cout << "Checking Consistency : ";
	if (history[0].size() == 0)
	{
		cout << "No history.\n";
		return;
	}

	for (int i = 0; i < num_threads; ++i)
	{
		for (auto& op : history[i])
		{
			if (false == op.o_value) continue;
			if (op.op == 3) continue;

			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}

	for (int i = 0; i < 1000; ++i)
	{
		int val = survive[i];

		if (val < 0)
		{
			cout << "The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1)
		{
			cout << "The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0)
		{
			if (my_set.CONTAINS(i))
			{
				cout << "The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1)
		{
			if (false == my_set.CONTAINS(i))
			{
				cout << "The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	cout << " OK\n";
}

int main()
{
	for (int num_threads = 1; num_threads <= 8; num_threads *= 2)
	{
		vector <thread> threads;
		array<vector <HISTORY>, 16> history;
		my_set.clear();

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &history[i], num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.print20();
		cout << num_threads << "Threads.  Exec Time : " << exec_ms << endl;

		check_history(history, num_threads);

		cout << endl;
	}

	cout << "===============================================================\n";

	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		vector <thread> threads;
		array<vector <HISTORY>, 16> history;
		my_set.clear();

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &history[i], num_threads);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		my_set.print20();
		cout << num_threads << "Threads.  Exec Time : " << exec_ms << endl;

		check_history(history, num_threads);

		cout << endl;
	}
}
