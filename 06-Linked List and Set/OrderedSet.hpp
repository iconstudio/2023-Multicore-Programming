#pragma once

struct LinkedNode
{
	constexpr LinkedNode(int value)
		: myValue(value)
		, myNext(), myPrev()
	{}

	void SetNext(LinkedNode* node)
	{
		myNext = node;
	}

	void SetPrev(LinkedNode* node)
	{
		myPrev = node;
	}

	int myValue;
	LinkedNode* myNext;
	LinkedNode* myPrev;
};

class VoidLock
{
public:
	void lock()
	{}

	void unlock()
	{}
};

class OrderedSet
{
public:
	OrderedSet()
		: myHead(new LinkedNode(std::numeric_limits<int>::min()))
		, myTail(new LinkedNode(std::numeric_limits<int>::max()))
		, myLock()
	{
		myHead->SetNext(myTail);
		myTail->SetPrev(myHead);
	}

	bool Add(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			result = true;
		}
		else
		{
			auto new_node = new LinkedNode(key);

			curr->SetPrev(new_node);
			new_node->SetNext(curr);

			prev->SetNext(new_node);
			new_node->SetPrev(prev);

			result = true;
		}

		myLock.unlock();
		return result;
	}

	bool Contains(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			result = true;
		}

		myLock.unlock();
		return result;
	}

	bool Remove(const int key)
	{
		bool result = false;

		LinkedNode* prev = myHead;
		myLock.lock();
		LinkedNode* curr = myHead->myNext;

		while (curr->myValue < key)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == key)
		{
			prev->SetNext(curr->myNext);

			if (curr->myNext)
			{
				curr->myNext->SetPrev(prev);
			}

			result = true;
		}

		myLock.unlock();
		return result;
	}

	inline void Clear()
	{
		myHead->SetNext(myTail);
		myTail->SetPrev(myHead);
	}

	LinkedNode* myHead;
	LinkedNode* myTail;

	// VoidLock
	std::mutex myLock;
};
