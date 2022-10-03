#pragma once

class LinkedNode : public std::enable_shared_from_this<LinkedNode>
{
public:
	LinkedNode(const int value)
		: myValue(value)
		, myLeft(nullptr), myRight(nullptr)
	{}

	~LinkedNode()
	{}

	inline std::shared_ptr<LinkedNode> AddNext(const int value)
	{
		auto new_node = std::make_shared<LinkedNode>(value);

		if (myRight)
		{
			myRight->SetBeforeRaw(new_node);
		}

		return SetNext(new_node);
	}

	inline std::shared_ptr<LinkedNode> SetNext(std::shared_ptr<LinkedNode> node)
	{
		if (myRight)
		{
			myRight->SetBeforeRaw(node);
		}

		return SetNextRaw(node);
	}

	inline std::shared_ptr<LinkedNode> SetBefore(std::shared_ptr<LinkedNode> node)
	{
		if (myLeft)
		{
			myLeft->SetNextRaw(node);
		}

		return SetBeforeRaw(node);
	}

	inline std::shared_ptr<LinkedNode> SetNextRaw(std::shared_ptr<LinkedNode> node)
	{
		return (myRight = node);
	}

	inline std::shared_ptr<LinkedNode> SetBeforeRaw(std::shared_ptr<LinkedNode> node)
	{
		return (myLeft = node);
	}

	inline std::shared_ptr<LinkedNode> GetNext()
	{
		return myRight;
	}

	inline std::shared_ptr<LinkedNode> GetBefore()
	{
		return myLeft;
	}

	const int myValue;
	std::shared_ptr<LinkedNode> myLeft, myRight;
};
