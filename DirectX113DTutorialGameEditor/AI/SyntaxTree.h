#pragma once

#include <vector>
#include <string>

struct SSyntaxTreeNode
{
	enum class EType
	{
		Directive,
		Literal,
		Operator,
		Identifier,

		Grouping
	};

	SSyntaxTreeNode() {}
	SSyntaxTreeNode(const std::string& _Identifier, EType _eType) : Identifier{ _Identifier }, eType{ _eType } {}
	SSyntaxTreeNode(const std::string& _Identifier, EType _eType, SSyntaxTreeNode* const _ParentNode) : 
		Identifier{ _Identifier }, eType{ _eType }, ParentNode{ _ParentNode } {}
	~SSyntaxTreeNode()
	{
		for (auto& ChildNode : vChildNodes)
		{
			if (ChildNode)
			{
				delete ChildNode;
				ChildNode = nullptr;
			}
		}
	}

	std::string						Identifier{};
	EType							eType{};
	SSyntaxTreeNode*				ParentNode{};
	std::vector<SSyntaxTreeNode*>	vChildNodes{};
};

class CSyntaxTree
{
public:
	CSyntaxTree();
	~CSyntaxTree();
	
public:
	static void MoveChildrenAsHead(SSyntaxTreeNode*& From, SSyntaxTreeNode*& To);
	static void MoveChildrenAsTail(SSyntaxTreeNode*& From, SSyntaxTreeNode*& To);
	static void MoveAsHead(SSyntaxTreeNode*& Src, SSyntaxTreeNode*& NewParent);
	static void MoveAsTail(SSyntaxTreeNode*& Src, SSyntaxTreeNode*& NewParent);
	static void Substitute(SSyntaxTreeNode*& Src, SSyntaxTreeNode* Dest);
	static void Substitute(const SSyntaxTreeNode& NewNode, SSyntaxTreeNode*& Dest);
	static void Remove(SSyntaxTreeNode* Node);

public:
	void Create(const SSyntaxTreeNode& RootNode);
	void CopyFrom(const SSyntaxTreeNode* const Node);
	void Destroy();

private:
	void _CopyFrom(SSyntaxTreeNode*& DestNode, SSyntaxTreeNode* DestParentNode, const SSyntaxTreeNode* const SrcNode);

public:
	void InsertChild(const SSyntaxTreeNode& Content);
	void GoToLastChild();
	void GoToParent();

public:
	bool IsCreated() const;
	const std::string& Serialize();
	SSyntaxTreeNode*& GetRootNode();

private:
	void SerializeTree(SSyntaxTreeNode* const CurrentNode, size_t Depth);

private:
	SSyntaxTreeNode*	m_SyntaxTreeRoot{};

private:
	SSyntaxTreeNode*	m_PtrCurrentNode{};
	std::string			m_SerializedTree{};
};
