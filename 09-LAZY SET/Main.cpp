#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <array>

using namespace std;
using namespace chrono;

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

class SHARED_MARKED_NODE
{
public:
	int v;
	shared_ptr<SHARED_MARKED_NODE> next;
	volatile bool removed;

	SHARED_MARKED_NODE()
		: SHARED_MARKED_NODE(-1)
	{}

	SHARED_MARKED_NODE(int x)
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
		auto p = head.next;

		while (p != &tail)
		{
			auto t = p;
			p = p->next;

			delete t;
		}
		head.next = &tail;
	}
};

class SHARED_LAZY_SET
{
	shared_ptr<SHARED_MARKED_NODE> head, tail;
public:
	SHARED_LAZY_SET()
		: head(make_shared<SHARED_MARKED_NODE>(0x80000000))
		, tail(make_shared<SHARED_MARKED_NODE>(0x7FFFFFFF))
	{
		//head->v = 0x80000000;
		//tail->v = 0x7FFFFFFF;
		head->next = tail;
		tail->next = nullptr;
	}

	bool Validate(const shared_ptr<SHARED_MARKED_NODE>& prev, const shared_ptr<SHARED_MARKED_NODE>& curr) const
	{
		return !prev->removed && !curr->removed && prev->next == curr;
	}

	bool ADD(int key)
	{
		while (true)
		{
			auto prev = head;
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
					auto node = make_shared<SHARED_MARKED_NODE>(key);
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
			auto prev = head;
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
		auto curr = head;

		while (curr->v < key)
		{
			curr = curr->next;
		}

		return curr->v == key && !curr->removed;
	}

	void print20()
	{
		auto it = head->next;
		for (int i = 0; i < 20; ++i)
		{
			if (it == tail) break;

			cout << it->v << ", ";
			it = it->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};

SHARED_LAZY_SET my_set{};
constexpr int target_summary = 400000;

class HISTORY
{
public:
	int op;
	int i_value; // input
	bool o_value; // output

	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re)
	{}
};

void worker(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < target_summary / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % 1000;
				my_set.ADD(v);
				break;
			}
			case 1:
			{
				int v = rand() % 1000;
				my_set.REMOVE(v);
				break;
			}
			case 2:
			{
				int v = rand() % 1000;
				my_set.CONTAINS(v);
				break;
			}
		}
	}
}

void worker_check(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < target_summary / num_threads; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
			case 0:
			{
				int v = rand() % 1000;
				history->emplace_back(0, v, my_set.ADD(v));
				break;
			}
			case 1:
			{
				int v = rand() % 1000;
				history->emplace_back(1, v, my_set.REMOVE(v));
				break;
			}
			case 2:
			{
				int v = rand() % 1000;
				history->emplace_back(2, v, my_set.CONTAINS(v));
				break;
			}
		}
	}
}

void check_history(array<vector<HISTORY>, 16>& history, int num_threads)
{
	array <int, 1000> survive = {};
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
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
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
