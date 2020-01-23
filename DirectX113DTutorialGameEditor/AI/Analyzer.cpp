#include "Analyzer.h"
#include "SyntaxTree.h"
#include "Tokenizer.h"

using std::vector;
using std::string;
using std::make_unique;

CAnalyzer::CAnalyzer()
{
}

CAnalyzer::~CAnalyzer()
{
}

void CAnalyzer::Analyze(const std::vector<std::string>& vTokens)
{
	m_vTokens = vTokens;
	
	m_SyntaxTree = make_unique<CSyntaxTree>();
	m_SyntaxTree->Create(SSyntaxTreeNode("root", SSyntaxTreeNode::EType::Identifier));
	

	BuildSyntaxTree();


	MakeGroupingNodes(m_SyntaxTree->GetRootNode(), "(", ")");
	MakeGroupingNodes(m_SyntaxTree->GetRootNode(), "{", "}");
	MakeGroupingNodes(m_SyntaxTree->GetRootNode(), "[", "]");


	OrganizeFunctionNodes(m_SyntaxTree->GetRootNode());
	OrganizeCastingNodes(m_SyntaxTree->GetRootNode());

	OrganizeUnaryOperatorNodes(m_SyntaxTree->GetRootNode(), "+");
	OrganizeUnaryOperatorNodes(m_SyntaxTree->GetRootNode(), "-");
	OrganizeUnaryOperatorNodes(m_SyntaxTree->GetRootNode(), "!");


	OrganizeDeclarationNodes(m_SyntaxTree->GetRootNode(), "real");
	OrganizeAssignmentNodes(m_SyntaxTree->GetRootNode());


	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "*");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "/");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "+");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "-");

	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "<");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "<=");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), ">");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), ">=");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "==");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "!=");

	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "&&");
	OrganizeBinaryOperatorNodes(m_SyntaxTree->GetRootNode(), "||");


	OrganizeNonCastingGroupingNodes(m_SyntaxTree->GetRootNode());
	

	RemoveUnnecessaryGroupingNodes(m_SyntaxTree->GetRootNode());
	RemovePunctuator(m_SyntaxTree->GetRootNode(), ";");
}

const std::string& CAnalyzer::Serialize()
{
	return m_SyntaxTree->Serialize();
}

SSyntaxTreeNode*& CAnalyzer::GetRootNode()
{
	return m_SyntaxTree->GetRootNode();
}

void CAnalyzer::BuildSyntaxTree()
{
	while (CanRead())
	{
		// #0 directive
		if (IsDirective())
		{
			m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Directive));
		}
		// #1 operator
		else if (IsOperator())
		{
			if (Compare("(") || Compare("{") || Compare("["))
			{
				m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Operator));
				m_SyntaxTree->GoToLastChild();
			}
			else if (Compare(")") || Compare("}") || Compare("]"))
			{
				m_SyntaxTree->GoToParent();
				m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Operator));
			}
			else
			{
				m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Operator));
			}
		}
		// #2 literal
		else if (IsLiteral())
		{
			m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Literal));
		}
		// #3 identifier
		else
		{
			m_SyntaxTree->InsertChild(SSyntaxTreeNode(GetToken(), SSyntaxTreeNode::EType::Identifier));
		}
		Skip();
	}
}

void CAnalyzer::MakeGroupingNodes(SSyntaxTreeNode*& CurrentNode, const std::string& Open, const std::string& Close)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		MakeGroupingNodes(ChildNode, Open, Close);
	}

	size_t iOpenNode{};
	while (iOpenNode < CurrentNode->vChildNodes.size())
	{
		size_t ChildCount{ CurrentNode->vChildNodes.size() };

		SSyntaxTreeNode*& OpenNode{ CurrentNode->vChildNodes[iOpenNode] };
		if (OpenNode->Identifier == Open)
		{
			// @important
			OpenNode->Identifier = Open + Close;
			OpenNode->eType = SSyntaxTreeNode::EType::Grouping;

			size_t iCloseNode{};
			for (size_t iNode = iOpenNode; iNode < ChildCount; ++iNode)
			{
				if (CurrentNode->vChildNodes[iNode]->Identifier == Close)
				{
					iCloseNode = iNode;
					break;
				}
			}
			assert(iCloseNode);

			for (size_t i = 0; i < iCloseNode - (iOpenNode + 1); ++i)
			{
				CSyntaxTree::MoveAsTail(CurrentNode->vChildNodes[iOpenNode + 1], OpenNode);
			}
			CurrentNode->vChildNodes.erase(CurrentNode->vChildNodes.begin() + iOpenNode + 1);
		}

		++iOpenNode;
	}
}

void CAnalyzer::RemoveUnnecessaryGroupingNodes(SSyntaxTreeNode*& CurrentNode)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		RemoveUnnecessaryGroupingNodes(ChildNode);
	}

	size_t iGroupingNode{};
	while (iGroupingNode < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& GroupingNode{ CurrentNode->vChildNodes[iGroupingNode] };
		if (GroupingNode->eType == SSyntaxTreeNode::EType::Grouping && 
			GroupingNode->vChildNodes.size() <= 1)
		{
			if (GroupingNode->vChildNodes.empty())
			{
				CSyntaxTree::Remove(GroupingNode);
			}
			else if (GroupingNode->vChildNodes.size() == 1)
			{
				CSyntaxTree::Substitute(GroupingNode->vChildNodes[0], GroupingNode);
			}
		}
		else
		{
			++iGroupingNode;
		}
	}
}

void CAnalyzer::OrganizeNonCastingGroupingNodes(SSyntaxTreeNode*& CurrentNode)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeNonCastingGroupingNodes(ChildNode);
	}

	size_t iGroupingNode{ 1 };
	while (iGroupingNode < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& GroupingNode{ CurrentNode->vChildNodes[iGroupingNode] };
		if (GroupingNode->eType == SSyntaxTreeNode::EType::Grouping && 
			GroupingNode->vChildNodes.size() &&
			!(GroupingNode->Identifier == "()" && GroupingNode->vChildNodes[0]->eType == SSyntaxTreeNode::EType::Directive)
			)
		{
			SSyntaxTreeNode*& PreGroupingNode{ CurrentNode->vChildNodes[iGroupingNode - 1] };
			CSyntaxTree::MoveAsTail(GroupingNode, PreGroupingNode);
		}
		else
		{
			++iGroupingNode;
		}
	}
}

void CAnalyzer::OrganizeFunctionNodes(SSyntaxTreeNode*& CurrentNode)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeFunctionNodes(ChildNode);
	}

	size_t iChild{};
	while (iChild + 1 < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& IdentifierNode{ CurrentNode->vChildNodes[iChild] };
		SSyntaxTreeNode*& ParenthesesNode{ CurrentNode->vChildNodes[iChild + 1] };
		if (IdentifierNode->eType == SSyntaxTreeNode::EType::Identifier &&
			ParenthesesNode->Identifier == "()")
		{
			size_t iArgumentNode{};
			while (iArgumentNode < ParenthesesNode->vChildNodes.size())
			{
				if (ParenthesesNode->vChildNodes[iArgumentNode]->Identifier == ",")
				{
					ParenthesesNode->vChildNodes.erase(ParenthesesNode->vChildNodes.begin() + iArgumentNode);
				}
				else
				{
					++iArgumentNode;
				}
			}
			if (ParenthesesNode->vChildNodes.empty())
			{
				ParenthesesNode->vChildNodes.emplace_back(new SSyntaxTreeNode("void", SSyntaxTreeNode::EType::Directive, ParenthesesNode));
			}
			CSyntaxTree::MoveChildrenAsTail(ParenthesesNode, IdentifierNode);
			CurrentNode->vChildNodes.erase(CurrentNode->vChildNodes.begin() + iChild + 1);
		}
		else
		{
			++iChild;
		}
	}
}

void CAnalyzer::OrganizeCastingNodes(SSyntaxTreeNode*& CurrentNode)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeCastingNodes(ChildNode);
	}

	size_t iCastingNode{};
	while (iCastingNode + 1 < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& CastingNode{ CurrentNode->vChildNodes[iCastingNode] };
		if (CastingNode->Identifier == "()" &&
			CastingNode->vChildNodes.size() == 1 &&
			CastingNode->vChildNodes[0]->eType == SSyntaxTreeNode::EType::Directive)
		{
			SSyntaxTreeNode*& PostCastingNode{ CurrentNode->vChildNodes[iCastingNode + 1] };
			CSyntaxTree::MoveAsTail(PostCastingNode, CastingNode);
		}
		else
		{
			++iCastingNode;
		}
	}
}

void CAnalyzer::OrganizeUnaryOperatorNodes(SSyntaxTreeNode*& CurrentNode, const std::string& UnaryOperator)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeUnaryOperatorNodes(ChildNode, UnaryOperator);
	}

	size_t iUnaryNode{};
	while (iUnaryNode + 1 < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& UnaryNode{ CurrentNode->vChildNodes[iUnaryNode] };
		SSyntaxTreeNode*& PostUnaryNode{ CurrentNode->vChildNodes[iUnaryNode + 1] };
		if (
			(UnaryNode->Identifier == UnaryOperator)
			&&
			(PostUnaryNode->eType == SSyntaxTreeNode::EType::Literal || PostUnaryNode->Identifier == "()")
			&&
			(iUnaryNode == 0 ? true : (CurrentNode->vChildNodes[iUnaryNode - 1]->eType == SSyntaxTreeNode::EType::Operator))
			)
		{
			CSyntaxTree::MoveAsTail(CurrentNode->vChildNodes[iUnaryNode + 1], UnaryNode);
		}
		else
		{
			++iUnaryNode;
		}
	}
}

void CAnalyzer::OrganizeBinaryOperatorNodes(SSyntaxTreeNode*& CurrentNode, const std::string& BinaryOperator)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeBinaryOperatorNodes(ChildNode, BinaryOperator);
	}

	size_t iBinaryNode{ 1 };
	while (iBinaryNode + 1 < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& BinaryNode{ CurrentNode->vChildNodes[iBinaryNode] };
		if (BinaryNode->Identifier == BinaryOperator)
		{
			SSyntaxTreeNode*& PreBinaryNode{ CurrentNode->vChildNodes[iBinaryNode - 1] };

			/*if (PreBinaryNode->Identifier == "=" ||
				PreBinaryNode->Identifier == "*=" ||
				PreBinaryNode->Identifier == "/=" ||
				PreBinaryNode->Identifier == "+=" ||
				PreBinaryNode->Identifier == "-=")
			{
				++iBinaryNode;
				continue;
			}*/

			CSyntaxTree::MoveAsTail(PreBinaryNode, BinaryNode);
			CSyntaxTree::MoveAsTail(BinaryNode, PreBinaryNode);
		}
		else
		{
			++iBinaryNode;
		}
	}
}

void CAnalyzer::OrganizeAssignmentNodes(SSyntaxTreeNode*& CurrentNode)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeAssignmentNodes(ChildNode);
	}

	size_t iChild{};
	while (iChild + 1 < CurrentNode->vChildNodes.size())
	{
		size_t ChildCount{ CurrentNode->vChildNodes.size() };

		SSyntaxTreeNode*& ChildNode0{ CurrentNode->vChildNodes[iChild] };
		SSyntaxTreeNode*& ChildNode1{ CurrentNode->vChildNodes[iChild + 1] };

		if (ChildNode0->eType == SSyntaxTreeNode::EType::Identifier &&
			ChildNode1->Identifier == "=")
		{
			size_t iSemicolonNode{};
			for (size_t iNode = iChild + 1; iNode < ChildCount; ++iNode)
			{
				if (CurrentNode->vChildNodes[iNode]->Identifier == ";")
				{
					iSemicolonNode = iNode;
					break;
				}
			}
			assert(iSemicolonNode);

			for (size_t i = 0; i < iSemicolonNode - (iChild + 2); ++i)
			{
				CSyntaxTree::MoveAsTail(CurrentNode->vChildNodes[iChild + 2], ChildNode1);
			}
			CSyntaxTree::MoveAsTail(ChildNode1, ChildNode0);
		}

		++iChild;
	}
}

void CAnalyzer::OrganizeDeclarationNodes(SSyntaxTreeNode*& CurrentNode, const std::string& TypeName)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		OrganizeDeclarationNodes(ChildNode, TypeName);
	}

	size_t iChild{};
	while (iChild + 1 < CurrentNode->vChildNodes.size())
	{
		size_t ChildCount{ CurrentNode->vChildNodes.size() };

		SSyntaxTreeNode*& ChildNode0{ CurrentNode->vChildNodes[iChild] };
		SSyntaxTreeNode* const ChildNode1{ CurrentNode->vChildNodes[iChild + 1] };

		if (ChildNode0->Identifier == TypeName &&
			ChildNode1->eType == SSyntaxTreeNode::EType::Identifier)
		{
			size_t iSemicolonNode{};
			for (size_t iNode = iChild + 1; iNode < ChildCount; ++iNode)
			{
				if (CurrentNode->vChildNodes[iNode]->Identifier == ";")
				{
					iSemicolonNode = iNode;
					break;
				}
			}
			//assert(iSemicolonNode);
			if (iSemicolonNode == 0)
			{
				++iChild;
				continue;
			}
			for (size_t i = 0; i <= iSemicolonNode - (iChild + 1); ++i)
			{
				CSyntaxTree::MoveAsTail(CurrentNode->vChildNodes[iChild + 1], ChildNode0);
			}
		}

		++iChild;
	}
}

void CAnalyzer::RemovePunctuator(SSyntaxTreeNode*& CurrentNode, const std::string& Punctuator)
{
	if (!CurrentNode) return;

	for (auto& ChildNode : CurrentNode->vChildNodes)
	{
		RemovePunctuator(ChildNode, Punctuator);
	}

	size_t iChild{};
	while (iChild < CurrentNode->vChildNodes.size())
	{
		SSyntaxTreeNode*& ChildNode{ CurrentNode->vChildNodes[iChild] };
		if (ChildNode->Identifier == Punctuator)
		{
			CurrentNode->vChildNodes.erase(CurrentNode->vChildNodes.begin() + iChild);
		}
		else
		{
			++iChild;
		}
	}
}

void CAnalyzer::AddDirective(const std::string& Directive)
{
	if (std::find(m_vDirectives.begin(), m_vDirectives.end(), Directive) == m_vDirectives.end())
	{
		m_vDirectives.emplace_back(Directive);
	}
}

void CAnalyzer::AddOperator(const std::string& Operator)
{
	if (std::find(m_vOperators.begin(), m_vOperators.end(), Operator) == m_vOperators.end())
	{
		m_vOperators.emplace_back(Operator);
	}
}

void CAnalyzer::AddLiteral(const std::string& Literal)
{
	size_t Find{ Literal.find("*") };
	if (Find != string::npos)
	{
		size_t Length{ Literal.size() };
		if (Length <= 1) return;
	}

	if (std::find(m_vLiterals.begin(), m_vLiterals.end(), Literal) == m_vLiterals.end())
	{
		m_vLiterals.emplace_back(Literal);
	}
}

bool CAnalyzer::IsKeyword(size_t Offset) const
{
	return IsDirective(Offset) || IsLiteral(Offset) || IsOperator(Offset);
}

bool CAnalyzer::IsDirective(size_t Offset) const
{
	for (const auto& Directive : m_vDirectives)
	{
		size_t Find{ Directive.find("*") };
		if (Find != string::npos)
		{
			size_t PrefixLength{ Directive.size() - Find };
			return (m_vTokens[m_TokenAt + Offset].substr(0, PrefixLength) == Directive.substr(0, PrefixLength));
		}
		else
		{
			if (m_vTokens[m_TokenAt + Offset] == Directive)
			{
				return true;
			}
		}
	}
	return false;
}

bool CAnalyzer::IsOperator(size_t Offset) const
{
	for (const auto& Operator : m_vOperators)
	{
		if (m_vTokens[m_TokenAt + Offset] == Operator)
		{
			return true;
		}
	}
	return false;
}

bool CAnalyzer::IsLiteral(size_t Offset) const
{
	for (const auto& Literal : m_vLiterals)
	{
		size_t Find{ Literal.find("*") };
		if (Find != string::npos)
		{
			size_t PrefixLength{ Literal.size() - Find };
			if (m_vTokens[m_TokenAt + Offset].substr(0, PrefixLength) == Literal.substr(0, PrefixLength))
			{
				return true;
			}
		}
		else
		{
			if (m_vTokens[m_TokenAt + Offset] == Literal)
			{
				return true;
			}
		}
	}
	return false;
}

bool CAnalyzer::CanRead(size_t Count, size_t Offset) const
{
	return (m_TokenAt + (Count - 1) + Offset < m_vTokens.size());
}

bool CAnalyzer::Compare(const std::string Cmp, size_t Offset) const
{
	if (!CanRead(Cmp.size(), Offset)) return false;

	return (m_vTokens[m_TokenAt + Offset] == Cmp);
}

const std::string& CAnalyzer::GetToken(size_t Offset) const
{
	return m_vTokens[m_TokenAt + Offset];
}

void CAnalyzer::Skip(size_t Count) const
{
	m_TokenAt += Count;
}
