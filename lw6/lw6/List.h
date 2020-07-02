#pragma once
#include <string>
#include <iostream>

class List
{
public:
	List();
	~List();
	size_t GetSize();
	void Push(std::string value);
	void Pop();
	std::string GetHead();
	void Clear();
private:
	struct Node
	{
		Node(std::string value, Node* ptr)
			:value(value)
			, next(ptr)
		{}
		std::string value;
		Node* next;
	};

	size_t m_ListSize = 0;
	Node* m_Head;
};