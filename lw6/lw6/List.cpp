#include "pch.h"
#include "List.h"

List::List()
	:m_ListSize(0)
	,m_Head(nullptr)
{}

List::~List()
{
	List::Clear();
}

size_t List::GetSize()
{
	return m_ListSize;
}

void List::Push(std::string value)
{
	Node* temp = new Node(value, nullptr);
	temp->next = m_Head;
	m_Head = temp;
	m_ListSize++;
}

void List::Pop()
{
	if(m_ListSize != 0)
	{
		Node* Head = m_Head->next;
		delete m_Head;
		m_Head = Head;
		m_ListSize--;
	}
}

std::string List::GetHead()
{
	return m_Head->value;
	
}

void List::Clear()
{
	while (m_ListSize != 0)
	{
		Pop();
	}
}