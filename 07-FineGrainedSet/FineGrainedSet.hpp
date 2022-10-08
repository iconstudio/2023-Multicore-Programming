#pragma once
#include <mutex>

struct LinkedNode
{
	LinkedNode(int value)
		: myValue(value)
		, myNext(), myLock()
	{}

	inline void SetNext(LinkedNode* node)
	{
		myNext = node;
	}

	inline void Lock()
	{
		myLock.lock();
	}

	inline void Unlock()
	{
		myLock.unlock();
	}

	const int myValue;

	LinkedNode* myNext;

private:
	std::mutex myLock;
};

class FineOrderedSet
{
public:
	FineOrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
	{
		myHead->SetNext(myTail);
	}

	bool Add(const int key)
	{
		bool result = false;

		myHead->Lock();
		LinkedNode* prev = myHead;
		LinkedNode* curr = prev->myNext;

		curr->Lock();
		while (curr->myValue < key)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		if (curr->myValue != key)
		{
			auto new_node = new LinkedNode(key);
			new_node->SetNext(curr);
			prev->SetNext(new_node);

			result = true;
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	bool Contains(const int key) const
	{
		bool result = false;

		myHead->Lock();
		LinkedNode* prev = myHead;
		LinkedNode* curr = prev->myNext;

		curr->Lock();
		while (curr->myValue < key)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		if (curr->myValue == key)
		{
			result = true;
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	bool Remove(const int key)
	{
		bool result = false;

		myHead->Lock();
		LinkedNode* prev = myHead;
		LinkedNode* curr = prev->myNext;

		curr->Lock();
		while (curr->myValue < key)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		if (curr->myValue == key)
		{
			prev->SetNext(curr->myNext);

			//delete curr;
			result = true;
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	inline void Clear()
	{
		myHead->SetNext(myTail);
	}

	LinkedNode* myHead;
	LinkedNode* myTail;
};

class OptimisticOrderedSet
{
public:
	OptimisticOrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
	{
		myHead->SetNext(myTail);
	}

	bool Validate(const int value, const LinkedNode* prev, const LinkedNode* curr)
	{
		auto it_prev = myHead;
		auto it = it_prev->myNext;

		while (it->myValue < value)
		{
			it_prev = it;
			it = it->myNext;
		}

		return (it_prev == prev && it == curr);
	}

	bool Add(const int value)
	{
		bool result = true;
		auto prev = myHead;
		auto curr = myHead->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();

		if (!Validate(value, prev, curr))
		{
			result = false;
		}
		else if (curr->myValue == value)
		{
			result = false;
		}
		else
		{
			auto new_node = new LinkedNode(value);

			new_node->SetNext(curr);

			prev->SetNext(new_node);
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	bool Contains(const int value)
	{
		bool result = false;
		auto prev = myHead;
		auto curr = prev->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();
		if (curr->myValue == value)
		{
			result = true;
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	void Remove(const int value)
	{
		auto prev = myHead;
		auto curr = myHead->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();

		if (Validate(value, prev, curr))
		{
			if (curr->myValue == value)
			{
				prev->SetNext(curr->myNext);

				curr->Unlock();
				prev->Unlock();

				//delete curr;
				return;
			}
		}

		curr->Unlock();
		prev->Unlock();
	}

	void Clear()
	{
		myHead->SetNext(myTail);
	}

	LinkedNode* myHead, * myTail;
};
