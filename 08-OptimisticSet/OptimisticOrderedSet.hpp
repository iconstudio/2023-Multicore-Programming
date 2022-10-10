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

class OptimisticOrderedSet
{
public:
	OptimisticOrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
	{
		myHead->SetNext(myTail);
	}

	bool Validate(const int key, const LinkedNode* prev, const LinkedNode* curr)
	{
		auto it_prev = myHead;
		auto it = it_prev->myNext;

		while (it->myValue < key)
		{
			it_prev = it;
			it = it->myNext;
		}

		return (it_prev == prev && it == curr);
	}

	bool Add(const int key)
	{
		bool result = true;

		while (true)
		{
			auto prev = myHead;
			auto curr = myHead->myNext;

			while (curr->myValue < key)
			{
				prev = curr;
				curr = curr->myNext;
			}

			prev->Lock();
			curr->Lock();

			if (!Validate(key, prev, curr))
			{
				result = false;
			}
			else if (curr->myValue == key)
			{
				result = false;
			}
			else
			{
				auto new_node = new LinkedNode(key);

				new_node->SetNext(curr);

				prev->SetNext(new_node);
			}

			curr->Unlock();
			prev->Unlock();

			return result;
		}
	}

	bool Contains(const int key)
	{
		bool result = false;
		auto prev = myHead;
		auto curr = prev->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();
		if (curr->myValue == key)
		{
			result = true;
		}

		curr->Unlock();
		prev->Unlock();

		return result;
	}

	void Remove(const int key)
	{
		auto prev = myHead;
		auto curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();

		if (Validate(key, prev, curr))
		{
			if (curr->myValue == key)
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
