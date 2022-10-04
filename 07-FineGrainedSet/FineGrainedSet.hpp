#pragma once
#include <memory>
#include <mutex>

struct LinkedNode
{
	LinkedNode(int value)
		: myValue(value)
		, myNext(), myPrev()
		, myLock()
	{}

	void SetNext(LinkedNode* node)
	{
		myNext = node;
	}

	void SetPrev(LinkedNode* node)
	{
		myPrev = node;
	}

	void Lock()
	{
		myLock.lock();
	}

	void Unlock()
	{
		myLock.unlock();
	}

	const int myValue;

	LinkedNode* myNext, * myPrev;

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

	inline bool Push(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();
		if (curr->myValue == value)
		{
			curr->Unlock();
			prev->Unlock();

			return false;
		}

		auto new_node = new LinkedNode(value);

		curr->SetPrev(new_node);
		new_node->SetNext(curr);

		prev->SetNext(new_node);
		new_node->SetPrev(prev);

		curr->Unlock();
		prev->Unlock();

		return true;
	}

	inline LinkedNode* Find(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr = prev->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();
		if (curr->myValue == value) // °ªÀ» Ã£¾Æ ³¿
		{
			curr->Unlock();
			prev->Unlock();
			return curr;
		}

		return myTail;
	}

	inline void Remove(const int value)
	{
		LinkedNode* prev = myHead;
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		prev->Lock();
		curr->Lock();
		if (curr->myValue == value) // °ªÀ» Ã£¾Æ ³¿
		{
			prev->SetNext(curr->myNext);

			curr->Unlock();
			prev->Unlock();

			//delete curr;
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

	inline LinkedNode* Push(const int value)
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
