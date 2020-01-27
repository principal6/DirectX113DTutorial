#include "SyntaxTree.h"
#include <cassert>

using std::vector;
using std::string;

CSyntaxTree::CSyntaxTree()
{
}

CSyntaxTree::~CSyntaxTree()
{
	Destroy();
}

void CSyntaxTree::MoveChildrenAsHead(SSyntaxTreeNode*& From, SSyntaxTreeNode*& To)
{
	if (!From || !To) return;
	if (From->vChildNodes.empty()) return;

	vector<SSyntaxTreeNode*> ToBackup{ To->vChildNodes };
	To->vChildNodes.clear();

	for (auto& iter : From->vChildNodes)
	{
		iter->ParentNode = To;
	}
	To->vChildNodes.assign(From->vChildNodes.begin(), From->vChildNodes.end());
	From->vChildNodes.clear();

	for (auto& iter : ToBackup)
	{
		To->vChildNodes.emplace_back(iter);
	}
	ToBackup.clear();
}

void CSyntaxTree::MoveChildrenAsTail(SSyntaxTreeNode*& From, SSyntaxTreeNode*& To)
{
	if (!From || !To) return;
	if (From->vChildNodes.empty()) return;

	for (auto& iter : From->vChildNodes)
	{
		iter->ParentNode = To; // @important
		To->vChildNodes.emplace_back(iter);
	}
	From->vChildNodes.clear();
}

void CSyntaxTree::MoveAsHead(SSyntaxTreeNode*& Src, SSyntaxTreeNode*& NewParent)
{
	if (!Src || !NewParent) return;
	if (!Src->ParentNode) return;

	SSyntaxTreeNode* SrcParentCopy{ Src->ParentNode };
	SSyntaxTreeNode* SrcCopy{ Src };

	bool bFound{ false };
	size_t iSrcNode{};
	for (iSrcNode = 0; iSrcNode < SrcParentCopy->vChildNodes.size(); ++iSrcNode)
	{
		if (SrcParentCopy->vChildNodes[iSrcNode] == Src)
		{
			bFound = true;
			break;
		}
	}
	assert(bFound);

	SrcCopy->ParentNode = NewParent;
	NewParent->vChildNodes.insert(NewParent->vChildNodes.begin(), SrcCopy);

	SrcParentCopy->vChildNodes.erase(SrcParentCopy->vChildNodes.begin() + iSrcNode);
}

void CSyntaxTree::MoveAsTail(SSyntaxTreeNode*& Src, SSyntaxTreeNode*& NewParent)
{
	if (!Src || !NewParent) return;
	if (!Src->ParentNode) return;

	SSyntaxTreeNode* SrcParentCopy{ Src->ParentNode };
	SSyntaxTreeNode* SrcCopy{ Src };

	bool bFound{ false };
	size_t iSrcNode{};
	for (iSrcNode = 0; iSrcNode < SrcParentCopy->vChildNodes.size(); ++iSrcNode)
	{
		if (SrcParentCopy->vChildNodes[iSrcNode] == Src)
		{
			bFound = true;
			break;
		}
	}
	assert(bFound);

	SrcCopy->ParentNode = NewParent;
	NewParent->vChildNodes.emplace_back(SrcCopy);

	SrcParentCopy->vChildNodes.erase(SrcParentCopy->vChildNodes.begin() + iSrcNode);
}

void CSyntaxTree::Substitute(SSyntaxTreeNode*& Src, SSyntaxTreeNode* Dest)
{
	if (!Src || !Dest) return;

	Src->ParentNode = Dest->ParentNode;

	bool bFound{ false };
	size_t iDestNode{};
	for (iDestNode = 0; iDestNode < Dest->ParentNode->vChildNodes.size(); ++iDestNode)
	{
		if (Dest->ParentNode->vChildNodes[iDestNode] == Dest)
		{
			bFound = true;
			break;
		}
	}
	assert(bFound);

	Dest->ParentNode->vChildNodes[iDestNode] = Src; // @important
	
	Dest->vChildNodes.clear();
	delete Dest;
	Dest = nullptr;
}

void CSyntaxTree::Substitute(const SSyntaxTreeNode& NewNode, SSyntaxTreeNode*& Dest)
{
	if (!Dest) return;

	Dest->eType = NewNode.eType;
	Dest->Identifier = NewNode.Identifier;
	
	for (auto& ChildNode : Dest->vChildNodes)
	{
		delete ChildNode;
		ChildNode = nullptr;
	}
	
	Dest->vChildNodes.clear();

	for (const auto& NewChildNode : NewNode.vChildNodes)
	{
		Dest->vChildNodes.emplace_back(NewChildNode);
	}
}

void CSyntaxTree::Remove(SSyntaxTreeNode* Node)
{
	if (!Node) return;

	bool bFound{ false };
	size_t iDestNode{};
	for (iDestNode = 0; iDestNode < Node->ParentNode->vChildNodes.size(); ++iDestNode)
	{
		if (Node->ParentNode->vChildNodes[iDestNode] == Node)
		{
			bFound = true;
			break;
		}
	}
	assert(bFound);

	Node->ParentNode->vChildNodes.erase(Node->ParentNode->vChildNodes.begin() + iDestNode);

	delete Node;
	Node = nullptr;
}

void CSyntaxTree::Create(const SSyntaxTreeNode& RootNode)
{
	Destroy();

	m_SyntaxTreeRoot = new SSyntaxTreeNode(RootNode);
	m_PtrCurrentNode = m_SyntaxTreeRoot;
}

void CSyntaxTree::CopyFrom(const SSyntaxTreeNode* const Node)
{
	Destroy();

	_CopyFrom(m_SyntaxTreeRoot, nullptr, Node);
}

void CSyntaxTree::Destroy()
{
	if (m_SyntaxTreeRoot)
	{
		delete m_SyntaxTreeRoot;
		m_SyntaxTreeRoot = nullptr;
	}
}

void CSyntaxTree::_CopyFrom(SSyntaxTreeNode*& DestNode, SSyntaxTreeNode* DestParentNode, const SSyntaxTreeNode* const SrcNode)
{
	if (!SrcNode) return;

	assert(!DestNode);

	DestNode = new SSyntaxTreeNode(*SrcNode);
	DestNode->ParentNode = DestParentNode;
	DestNode->vChildNodes.clear();

	for (const auto& ChildNode : SrcNode->vChildNodes)
	{
		DestNode->vChildNodes.emplace_back();

		_CopyFrom(DestNode->vChildNodes.back(), DestNode, ChildNode);
	}
}

void CSyntaxTree::InsertChild(const SSyntaxTreeNode& Content)
{
	SSyntaxTreeNode* NewNode{ new SSyntaxTreeNode(Content) };

	NewNode->ParentNode = m_PtrCurrentNode; // @important

	m_PtrCurrentNode->vChildNodes.emplace_back(NewNode);
}

void CSyntaxTree::GoToLastChild()
{
	if (m_PtrCurrentNode)
	{
		if (m_PtrCurrentNode->vChildNodes.size())
		{
			m_PtrCurrentNode = m_PtrCurrentNode->vChildNodes.back();
		}
	}
}

void CSyntaxTree::GoToParent()
{
	if (m_PtrCurrentNode && m_PtrCurrentNode->ParentNode) // @important: root node must be unique
	{
		m_PtrCurrentNode = m_PtrCurrentNode->ParentNode;
	}
}

bool CSyntaxTree::IsCreated() const
{
	return ((m_SyntaxTreeRoot) ? true : false);
}

const std::string& CSyntaxTree::Serialize()
{
	m_SerializedTree.clear();
	SerializeTree(m_SyntaxTreeRoot, 0);
	return m_SerializedTree;
}

void CSyntaxTree::SerializeTree(SSyntaxTreeNode* const CurrentNode, size_t Depth)
{
	if (CurrentNode)
	{
		if (CurrentNode == m_SyntaxTreeRoot) m_SerializedTree.clear();

		if (Depth > 0)
		{
			if (Depth == 1)
			{
				m_SerializedTree += "+-- ";
			}
			else
			{
				for (size_t iDepth = 0; iDepth < Depth - 1; ++iDepth)
				{
					m_SerializedTree += "    ";
				}
				m_SerializedTree += "+-- ";
			}
		}
		m_SerializedTree += CurrentNode->Identifier;

		switch (CurrentNode->eType)
		{
		case SSyntaxTreeNode::EType::Directive:
			m_SerializedTree += "     *DIR";
			break;
		case SSyntaxTreeNode::EType::Operator:
			m_SerializedTree += "     *OPR";
			break;
		case SSyntaxTreeNode::EType::Literal:
			m_SerializedTree += "     *LIT";
			break;
		case SSyntaxTreeNode::EType::Identifier:
			m_SerializedTree += "     *IDN";
			break;
		case SSyntaxTreeNode::EType::Grouping:
			m_SerializedTree += "     *GRP";
			break;
		default:
			break;
		}

		m_SerializedTree += '\n';

		for (const auto& ChildNode : CurrentNode->vChildNodes)
		{
			SerializeTree(ChildNode, Depth + 1);
		}
	}
}

SSyntaxTreeNode*& CSyntaxTree::GetRootNode()
{
	return m_SyntaxTreeRoot;
}
