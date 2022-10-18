#pragma once
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <array>

using namespace std;
using namespace chrono;

class LOCKFREE_PTR;
class LOCKFREE_NODE
{
public:
	LOCKFREE_NODE()
		: LOCKFREE_NODE(-1)
	{}

	LOCKFREE_NODE(int key)
		: LOCKFREE_NODE(false, nullptr)
	{}

	LOCKFREE_NODE(int key, LOCKFREE_NODE* ptr)
		: v(key)
		, next(false, ptr)
	{}

	int v;
	LOCKFREE_PTR next;
};

using LF_HANDLE = unsigned long long;
class LOCKFREE_PTR
{
public:
	LOCKFREE_PTR()
		: LOCKFREE_PTR(false, nullptr)
	{}

	LOCKFREE_PTR(bool mark, LOCKFREE_NODE* node_handle)
		: next(0)
	{
		next = reinterpret_cast<LF_HANDLE>(node_handle);

		if (mark) // ������ ��Ʈ�� 1
		{
			next |= 1;
		}
	}

	// get_pointer
	LOCKFREE_NODE* GetNodePtr()
	{
		//return reinterpret_cast<LOCKFREE_NODE*>((next >> 1) << 1);
		return reinterpret_cast<LOCKFREE_NODE*>(next & 0xFFFFFFFFFFFFFFFE);
	}

	// get_removed
	bool GetRemoved()
	{
		return (next & 1) == 1;
	}

	// get_pointer_mark
	LOCKFREE_NODE* GetRemovedPointer(bool& result)
	{
		LF_HANDLE current_next = next;
		result = (current_next & 1) == 1;

		// ���������� �����͸� ��ȯ�Ѵ�.
		// ����: ���ٿ��� ��ŷ�� �ߴµ��� ���� next �ʵ忡 ������ �ȵǾ��� ���� �ִ�.
		return reinterpret_cast<LOCKFREE_NODE*>(current_next & 0xFFFFFFFFFFFFFFFE);
	}

	// CAS
	bool CAS(LOCKFREE_NODE* old_ptr, LOCKFREE_NODE* new_ptr, bool old_mark, bool new_mark)
	{
		// �� ��
		LF_HANDLE old_next = reinterpret_cast<LF_HANDLE>(old_ptr);
		if (old_mark)
		{
			old_next++;
		}

		// �ٲ� ��
		LF_HANDLE new_next = reinterpret_cast<LF_HANDLE>(new_ptr);
		if (new_mark)
		{
			new_next++;
		}

		// &next�� ����ȯ
		return std::atomic_compare_exchange_strong
		(
			reinterpret_cast<std::atomic_uint64_t*>(&next)
			, &old_next, new_next
		);
	}

	LF_HANDLE next;
};

class LOCKFREE_SET
{
	LOCKFREE_NODE head;
	LOCKFREE_NODE tail;

public:
	LOCKFREE_SET()
		: head(), tail()
	{
		head = LOCKFREE_NODE{ 0x80000000, &tail };
		//head.next = LOCKFREE_PTR{ false, &tail };

		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
	}

	void FIND(LOCKFREE_NODE*& prev, LOCKFREE_NODE*& curr, const int key)
	{
		while (true)
		{
			prev = &head;
			curr = prev->next.GetNodePtr();

			// ó���� �����Ǿ��ٰ� ǥ�õ� ������ û����
		retry:
			while (true)
			{
				// ���� �õ�
				bool removed = false;

				// ���� ����� ��¥ ������
				auto successor = curr->next.GetRemovedPointer(removed);

				// if �����ְ� ��� �ݺ������� !removed�� �� ���� ��带 �����ؾ� �Ѵ�.
				// if (removed)

				while (!removed)
				{
					// ���� �õ�
					// ���ڿ� curr->next.GetNodePtr()�� �ٷ� ���� ���� prev�� ���� �̸� ���Ѵ�.
					if (!prev->next.CAS(curr, curr->next.GetNodePtr(), false, false))
					{
						// if ���� break�� ���... �׷��� ���������� ���ϹǷ� �ȵ�!
						// break;
						goto retry;
					}

					// ���� ���� ������
					curr = successor;
				}

				// ���⼭ �Լ� ����
				if (key <= curr->v)
				{
					return;
				}

				// ��� ��ȸ
				prev = curr;
				curr = successor;
			}
		}
	}

	bool ADD(int key)
	{
		while (true)
		{
			LOCKFREE_NODE* prev = &head;
			LOCKFREE_NODE* curr = prev->next.GetNodePtr();

			FIND(prev, curr, key);

			if (curr->v != key)
			{
				auto node = new LOCKFREE_NODE{ key, curr };
				if (prev->next.CAS(curr, node, false, false))
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
		while (true)
		{
			LOCKFREE_NODE* prev = &head;
			LOCKFREE_NODE* curr = prev->next.GetNodePtr();

			FIND(prev, curr, key);

			if (curr->v != key)
			{
				return false;
			}
			else
			{
				prev->next = curr->next;

				return true;
			}
		}
	}

	bool CONTAINS(int key)
	{
		auto curr = head.next.GetNodePtr();

		while (curr->v < key)
		{
			curr = curr->next.GetNodePtr();
		}

		return curr->v == key && curr->next.GetRemoved();
	}

	void print20()
	{
		auto it = head.next.GetNodePtr();

		for (int i = 0; i < 20; ++i)
		{
			if (it == &tail) break;

			cout << it->v << ", ";

			it = it->next.GetNodePtr();
		}

		cout << endl;
	}

	void clear()
	{
		auto it = head.next.GetNodePtr();

		while (it != &tail)
		{
			auto t = it;

			it = it->next.GetNodePtr();

			delete t;
		}

		head.next = LOCKFREE_PTR{ false, &tail };
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

	inline void lock()
	{
		n_lock.lock();
	}

	inline void unlock()
	{
		n_lock.unlock();
	}

	inline void mark()
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

	inline void lock()
	{
		n_lock.lock();
	}

	inline void unlock()
	{
		n_lock.unlock();
	}

	inline void mark()
	{
		removed = true;
	}

private:
	mutex n_lock;
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
