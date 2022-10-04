#pragma once

struct LinkedNode
{
	constexpr LinkedNode(int value)
		: myValue(value)
		, myNext(), myPrev()
	{}

	void SetNext(std::shared_ptr<LinkedNode> node)
	{
		myNext = node;
	}

	void SetPrev(std::shared_ptr<LinkedNode> node)
	{
		myPrev = node;
	}

	int myValue;
	std::shared_ptr<LinkedNode> myNext, myPrev;
};

class OrderedSet
{
public:
	OrderedSet()
		: myHead(std::make_shared<LinkedNode>(std::numeric_limits<int>::min()))
		, myTail(std::make_shared<LinkedNode>(std::numeric_limits<int>::max()))
		, myLock()
	{
		myHead->SetNext(myTail);
		myTail->SetPrev(myHead);
	}

	inline std::shared_ptr<LinkedNode> Push(const int value)
	{
		myLock.lock();

		auto curr = Find(value);

		if (myTail == curr)
		{
			auto prev = curr->myPrev;
			auto new_node = std::make_shared<LinkedNode>(value);

			curr->SetPrev(new_node);
			new_node->SetNext(curr);

			prev->SetNext(new_node);
			new_node->SetPrev(prev);

			std::cout << "삽입 성공, 값: " << value << ".\n";

			myLock.unlock();
			return new_node;
		}
		else // 중복 값은 금지
		{
			std::cout << "중복 값: " << value << ".\n";

			myLock.unlock();
		}

		return curr;
	}

	inline std::shared_ptr<LinkedNode> Find(const int value)
	{
		std::shared_ptr<LinkedNode> prev, curr = myHead;

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->myNext;
		}

		if (curr->myValue == value) // 값을 찾아 냄
		{
			return curr;
		}

		return myTail;
	}

	inline std::shared_ptr<LinkedNode> Pop(const int value)
	{
		auto target = Find(value);

		if (target)
		{
			auto& after = target->myNext;
			auto& before = target->myPrev;
			if (after)
			{
				after->SetPrev(before);
			}
			if (before)
			{
				before->SetNext(after);
			}
		}
		else
		{
			return nullptr;
		}
	}

	inline void Clear()
	{
		myHead->SetNext(myTail);
	}

	const std::shared_ptr<LinkedNode> myHead, myTail;
	std::mutex myLock;
};
