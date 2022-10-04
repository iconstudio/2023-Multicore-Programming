#pragma once
#include <mutex>

struct LinkedNode
{
	LinkedNode(int value)
		: myValue(value)
		, myNext(), myPrev(), myLock()
	{}

	inline void SetNext(LinkedNode* node)
	{
		myNext = node;
	}

	inline void SetPrev(LinkedNode* node)
	{
		myPrev = node;
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
	LinkedNode* myPrev;

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
		myTail->SetPrev(myHead);
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

			curr->SetPrev(new_node);
			new_node->SetNext(curr);

			prev->SetNext(new_node);
			new_node->SetPrev(prev);
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
		myTail->SetPrev(myHead);
	}

	LinkedNode* myHead, * myTail;
};

class FineOrderedSet
{
public:
	FineOrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
	{
		myHead->SetNext(myTail);
		myTail->SetPrev(myHead);
	}

	inline LinkedNode* Add(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr;

		prev->Lock();

		curr = myHead->myNext;
		curr->Lock();

		while (curr->myValue < value)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		if (curr->myValue == value) // °ªÀ» Ã£¾Æ ³¿
		{
			curr->Unlock();
			prev->Unlock();

			return curr;
		}

		auto new_node = new LinkedNode(value);

		curr->SetPrev(new_node);
		new_node->SetNext(curr);

		prev->SetNext(new_node);
		new_node->SetPrev(prev);

		curr->Unlock();
		prev->Unlock();

		return curr;
	}

	inline LinkedNode* Find(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr;

		prev->Lock();

		curr = prev->myNext;
		curr->Lock();

		while (curr->myValue < value)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		curr->Unlock();
		prev->Unlock();

		if (curr->myValue == value) // °ªÀ» Ã£¾Æ ³¿
		{
			return curr;
		}

		return myTail;
	}

	inline void Remove(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr;

		prev->Lock();

		curr = myHead->myNext;
		curr->Lock();

		while (curr->myValue < value)
		{
			prev->Unlock();
			prev = curr;
			curr = curr->myNext;
			curr->Lock();
		}

		if (curr->myValue == value) // °ªÀ» Ã£¾Æ ³¿
		{
			prev->SetNext(curr->myNext);

			curr->Unlock();
			delete curr;
		}

		prev->Unlock();
	}

	inline void Clear()
	{
		myHead->SetNext(myTail);
		myTail->SetPrev(myHead);
	}

	LinkedNode* myHead, * myTail;
};
