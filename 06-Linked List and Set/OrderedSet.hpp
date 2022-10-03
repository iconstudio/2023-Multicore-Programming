#pragma once
#include "SetNode.hpp"

class OrderedSet
{
public:
	OrderedSet()
		: myHead(std::make_shared<LinkedNode>(std::numeric_limits<int>::min()))
		, myTail(std::make_shared<LinkedNode>(std::numeric_limits<int>::max()))
		, myLock()
	{
		myHead->SetNext(myTail);
	}

	OrderedSet(std::initializer_list<int> list)
		: OrderedSet()
	{
		for (auto& value : list)
		{
			Push(value);
		}
	}

	template<std::forward_iterator It, std::sentinel_for<It> Guard>
	OrderedSet(It itbegin, Guard itend)
		: OrderedSet()
	{
		for (auto it = itbegin; it != itend; it++)
		{
			Push(*it);
		}
	}

	inline std::shared_ptr<LinkedNode> Push(const int value)
	{
		std::shared_ptr<LinkedNode> prev{}, curr = myHead;

		myLock.lock();

		while (curr->myValue < value)
		{
			prev = curr;
			curr = curr->GetNext();
		}

		if (curr->myValue == value) // 중복 값은 금지
		{
			std::cout << "중복 값은 넣을 수 없음, 값: " << value << ".\n";

			myLock.unlock();
		}
		else
		{
			//auto new_node = curr->AddNext(value);

			auto new_node = std::make_shared<LinkedNode>(value);

			curr->SetBeforeRaw(new_node);
			prev->SetNext(curr);

			//std::cout << "마지막 부분에 삽입 성공, 값: " << value << ".\n";

			myLock.unlock();
			return new_node;
		}

		return nullptr;
	}

	inline std::shared_ptr<LinkedNode> Find(const int value)
	{
		std::shared_ptr<LinkedNode> next = myHead;
		std::shared_ptr<LinkedNode> it{};

		while (true)
		{
			it = next;
			next = myHead->myRight;

			if (it->myValue == value) // 값을 찾아 냄
			{
				return it;
			}
			else if (!next) // 마지막 노드
			{
				break;
			}
		}
	}

	inline std::shared_ptr<LinkedNode> Pop(const int value)
	{
		auto target = Find(value);

		if (target)
		{
			auto before = target->GetBefore();
			if (before)
			{
				before->SetNext(target->GetNext());
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

	std::shared_ptr<LinkedNode> myHead, myTail;
	std::mutex myLock;
};
