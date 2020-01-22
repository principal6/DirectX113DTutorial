#include "Pattern.h"
#include "Analyzer.h"
#include "SyntaxTree.h"
#include "Tokenizer.h"

#include <fstream>
#include <cmath>
#include <ctime>

using std::vector;
using std::string;
using std::ifstream;
using std::swap;
using std::make_unique;
using std::stof;
using std::to_string;

CPattern::CPattern()
{
}

CPattern::~CPattern()
{
}

void CPattern::Load(const char* FileName)
{
	srand((unsigned int)time(nullptr));

	CTokenizer Tokenizer{};
	CAnalyzer Analyzer{};

	{
		Tokenizer.AddCondition(SCondition("//", SCondition::EToDo::SkipLine));
		Tokenizer.AddDivider(' ');
		Tokenizer.AddDivider('\t');
		Tokenizer.AddDivider('\n');

		Tokenizer.AddDivider(',');
		Tokenizer.AddDivider(';');

		Tokenizer.AddDivider("<=");
		Tokenizer.AddDivider(">=");
		Tokenizer.AddDivider("==");
		Tokenizer.AddDivider("!=");
		Tokenizer.AddDivider("&&");
		Tokenizer.AddDivider("||");
		Tokenizer.AddDivider("*=");
		Tokenizer.AddDivider("/=");
		Tokenizer.AddDivider("+=");
		Tokenizer.AddDivider("-=");
		Tokenizer.AddDivider("(");
		Tokenizer.AddDivider(")");
		Tokenizer.AddDivider("{");
		Tokenizer.AddDivider("}");
		Tokenizer.AddDivider("[");
		Tokenizer.AddDivider("]");
		Tokenizer.AddDivider("+");
		Tokenizer.AddDivider("-");
		Tokenizer.AddDivider("!");
		Tokenizer.AddDivider("*");
		Tokenizer.AddDivider("/");
		Tokenizer.AddDivider("<");
		Tokenizer.AddDivider(">");
		Tokenizer.AddDivider("=");

		Tokenizer.Tokenize(FileName);

		Tokenizer.EraseTokens(' ');
		Tokenizer.EraseTokens('\t');
		Tokenizer.EraseTokens('\n');
	}

	{
		Analyzer.AddDirective("#state");
		Analyzer.AddDirective("if");
		Analyzer.AddDirective("else");
		Analyzer.AddDirective(";"); // punctuator

		Analyzer.AddLiteral("0*");
		Analyzer.AddLiteral("1*");
		Analyzer.AddLiteral("2*");
		Analyzer.AddLiteral("3*");
		Analyzer.AddLiteral("4*");
		Analyzer.AddLiteral("5*");
		Analyzer.AddLiteral("6*");
		Analyzer.AddLiteral("7*");
		Analyzer.AddLiteral("8*");
		Analyzer.AddLiteral("9*");
		Analyzer.AddLiteral("true");
		Analyzer.AddLiteral("false");
		
		Analyzer.AddOperator(",");

		Analyzer.AddOperator("(");
		Analyzer.AddOperator(")");
		Analyzer.AddOperator("{");
		Analyzer.AddOperator("}");
		Analyzer.AddOperator("[");
		Analyzer.AddOperator("]");
		Analyzer.AddOperator("+");
		Analyzer.AddOperator("-");
		Analyzer.AddOperator("!");
		Analyzer.AddOperator("*");
		Analyzer.AddOperator("/");
		Analyzer.AddOperator("<");
		Analyzer.AddOperator("<=");
		Analyzer.AddOperator(">");
		Analyzer.AddOperator(">=");
		Analyzer.AddOperator("==");
		Analyzer.AddOperator("!=");
		Analyzer.AddOperator("&&");
		Analyzer.AddOperator("||");
		Analyzer.AddOperator("=");
		Analyzer.AddOperator("*=");
		Analyzer.AddOperator("/=");
		Analyzer.AddOperator("+=");
		Analyzer.AddOperator("-=");

		Analyzer.Analyze(Tokenizer.GetTokens());
	}
	
	const auto& Serialized{ Analyzer.Serialize() };
	auto& RootNode{ Analyzer.GetRootNode() };

	m_SyntaxTree = make_unique<CSyntaxTree>();
	m_SyntaxTree->CopyFrom(RootNode);
	m_InstructionSyntaxTree = make_unique<CSyntaxTree>();
	m_umapStateNameToID.clear();
	for (const auto& StateNode : m_SyntaxTree->GetRootNode()->vChildNodes)
	{
		if (StateNode->Identifier == "#state")
		{
			m_umapStateNameToID[StateNode->Identifier] = m_StateCount;
			++m_StateCount;
		}
	}
}

const SSyntaxTreeNode* CPattern::Execute(SPatternState& PatternState)
{
	if (!m_SyntaxTree) return nullptr;
	if (!m_SyntaxTree->GetRootNode()) return nullptr;

	m_InternalState.StateID = PatternState.StateID;
	m_InternalState.MyPosition = PatternState.MyPosition;
	m_InternalState.EnemyPosition = PatternState.EnemyPosition;

	const auto& StateNode{ m_SyntaxTree->GetRootNode()->vChildNodes[PatternState.StateID] };
	const auto& GroupingNode{ StateNode->vChildNodes.back() };

	size_t ControlNodeCount{ GroupingNode->vChildNodes.size() };
	for (size_t iControlNode = 0; iControlNode < ControlNodeCount; ++iControlNode)
	{
		auto& ControlNode{ GroupingNode->vChildNodes[iControlNode] };
		if (ControlNode->Identifier == "else" && iControlNode == ControlNodeCount - 1) // last else
		{
			ExecuteInstructionNode(ControlNode->vChildNodes.back(), PatternState.InstructionIndex);
		}
		if (ControlNode->Identifier == "if") // if, else if
		{
			// process if
			bool IfResult{ ExecuteIfNode(ControlNode) }; 
			if (IfResult)
			{
				ExecuteInstructionNode(ControlNode->vChildNodes.back(), PatternState.InstructionIndex);
			}

			// process else if
			if (iControlNode + 1 < ControlNodeCount)
			{
				auto& NextControlNode{ GroupingNode->vChildNodes[iControlNode + 1] };
				if (NextControlNode->Identifier == "else")
				{
					if (IfResult == true) break;
				}
			}
		}
	}

	PatternState.StateID = m_InternalState.StateID;

	return m_InstructionSyntaxTree->GetRootNode();
}

bool CPattern::ExecuteIfNode(const SSyntaxTreeNode* const IfNode)
{
	if (!IfNode) return false;
	if (IfNode->vChildNodes.empty()) return false;

	CSyntaxTree Tree{};
	Tree.CopyFrom(IfNode);

	auto& OperatorNode{ Tree.GetRootNode()->vChildNodes[0] };
	assert(OperatorNode->vChildNodes.size() == 2);
	for (auto& SideNode : OperatorNode->vChildNodes)
	{
		_ExecuteIfNode(SideNode);
	}
	float Left{ stof(OperatorNode->vChildNodes[0]->Identifier) };
	float Right{ stof(OperatorNode->vChildNodes[1]->Identifier) };

	if (OperatorNode->Identifier == "==")
	{
		return Left == Right;
	}
	else if (OperatorNode->Identifier == "!=")
	{
		return Left != Right;
	}
	else if (OperatorNode->Identifier == ">=")
	{
		return Left >= Right;
	}
	else if (OperatorNode->Identifier == ">")
	{
		return Left > Right;
	}
	else if (OperatorNode->Identifier == "<=")
	{
		return Left <= Right;
	}
	else if (OperatorNode->Identifier == "<")
	{
		return Left < Right;
	}

	return false;
}

void CPattern::_ExecuteIfNode(SSyntaxTreeNode*& Node)
{
	if (!Node) return;

	if (Node->eType == SSyntaxTreeNode::EType::Operator)
	{
		_ExecuteIfNode(Node);
	}

	if (Node->eType == SSyntaxTreeNode::EType::Identifier)
	{
		if (Node->vChildNodes.empty())
		{
			// variable

			float Value{ GetVariableValue(Node->Identifier) };
			CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Value), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
		else
		{
			// function

			CSyntaxTree Tree{};
			Tree.CopyFrom(Node);
			ExecuteFunctionNode(Tree.GetRootNode());
			CSyntaxTree::Substitute(SSyntaxTreeNode(Tree.GetRootNode()->Identifier, SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
	}
}

void CPattern::ExecuteFunctionNode(SSyntaxTreeNode*& Node)
{
	if (!Node) return;
	if (Node->vChildNodes.empty()) return;

	for (auto& Argument : Node->vChildNodes)
	{
		if (Argument->vChildNodes.size())
		{
			// function node
			ExecuteFunctionNode(Argument);
		}
		else
		{
			// variable node
			if (Argument->eType != SSyntaxTreeNode::EType::Literal)
			{
				float Value{ GetVariableValue(Argument->Identifier) };
				Argument->Identifier = to_string(Value);
				Argument->eType = SSyntaxTreeNode::EType::Literal;
			}
		}
	}

	if (Node->Identifier == "random")
	{
		assert(Node->vChildNodes.size() == 2);

		float Min{ stof(Node->vChildNodes[0]->Identifier) };
		float Max{ stof(Node->vChildNodes[1]->Identifier) };

		float Range{ Max - Min };

		float Random{ static_cast<float>((double)rand() / (double)RAND_MAX) };
		Random *= Range;
		Random += Min;

		CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Random), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
	}
	else if (Node->Identifier == "set_state")
	{
		m_InternalState.StateID = m_umapStateNameToID.at(Node->vChildNodes[0]->Identifier);
	}
}

void CPattern::ExecuteInstructionNode(const SSyntaxTreeNode* const ExecutionNode, size_t& InstructionIndex)
{
	if (!ExecutionNode) return;
	assert(InstructionIndex < ExecutionNode->vChildNodes.size());

	if (InstructionIndex == 0)
	{
		m_StackCount = 0; // @important
		m_umapStackVariableNameToID.clear();
	}

	const auto& CurrentNode{ ExecutionNode->vChildNodes[InstructionIndex] };
	const auto& FirstChildNode{ CurrentNode->vChildNodes.front() };
	if (FirstChildNode->Identifier == "=" ||
		FirstChildNode->Identifier == "+=" ||
		FirstChildNode->Identifier == "-=" ||
		FirstChildNode->Identifier == "*=" ||
		FirstChildNode->Identifier == "/=")
	{
		// variable

		m_InstructionSyntaxTree->CopyFrom(CurrentNode);
		ExecuteVariableNode(m_InstructionSyntaxTree->GetRootNode());

		if (m_umapStackVariableNameToID.find(CurrentNode->Identifier) != m_umapStackVariableNameToID.end())
		{
			size_t StackIndex{ m_umapStackVariableNameToID.at(CurrentNode->Identifier) };
			m_Stack[StackIndex] = stof(m_InstructionSyntaxTree->GetRootNode()->Identifier);
		}
		else
		{
			m_Stack[m_StackCount] = stof(m_InstructionSyntaxTree->GetRootNode()->Identifier);
			m_umapStackVariableNameToID[CurrentNode->Identifier] = m_StackCount;

			++m_StackCount;
		}
	}
	else
	{
		// function

		m_InstructionSyntaxTree->CopyFrom(CurrentNode);
		ExecuteFunctionNode(m_InstructionSyntaxTree->GetRootNode());
	}

	++InstructionIndex;
	if (InstructionIndex >= ExecutionNode->vChildNodes.size()) InstructionIndex = 0;
}

void CPattern::ExecuteVariableNode(SSyntaxTreeNode*& Node)
{
	if (!Node) return;

	if (Node->eType == SSyntaxTreeNode::EType::Identifier && 
		(Node->vChildNodes.empty() || (Node->vChildNodes.size() && Node->vChildNodes[0]->eType != SSyntaxTreeNode::EType::Operator)))
	{
		// function or variable node

		size_t ChildCount{ Node->vChildNodes.size() };
		if (ChildCount == 0)
		{
			// variable

			float Value{ GetVariableValue(Node->Identifier) };
			CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Value), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
		else
		{
			// function

			CSyntaxTree Tree{};
			Tree.CopyFrom(Node);
			ExecuteFunctionNode(Tree.GetRootNode());
			CSyntaxTree::Substitute(SSyntaxTreeNode(Tree.GetRootNode()->Identifier, SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
	}
	else
	{
		// literal

		size_t ChildCount{ Node->vChildNodes.size() };
		if (ChildCount == 1)
		{
			// unary
			if (Node->vChildNodes[0]->eType != SSyntaxTreeNode::EType::Literal)
			{
				ExecuteVariableNode(Node->vChildNodes[0]);
			}

			float Result{ stof(Node->vChildNodes[0]->Identifier) };
			if (Node->Identifier == "-")
			{
				Result = -Result;
			}

			CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Result), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
		else if (ChildCount == 2)
		{
			// binary
			if (Node->vChildNodes[0]->eType != SSyntaxTreeNode::EType::Literal)
			{
				ExecuteVariableNode(Node->vChildNodes[0]);
			}
			if (Node->vChildNodes[1]->eType != SSyntaxTreeNode::EType::Literal)
			{
				ExecuteVariableNode(Node->vChildNodes[1]);
			}

			float Left{ stof(Node->vChildNodes[0]->Identifier) };
			float Right{ stof(Node->vChildNodes[1]->Identifier) };

			float Result{};
			if (Node->Identifier == "+")
			{
				Result = Left + Right;
			}
			else if (Node->Identifier == "-")
			{
				Result = Left - Right;
			}
			else if (Node->Identifier == "*")
			{
				Result = Left * Right;
			}
			else if (Node->Identifier == "/")
			{
				Result = Left / Right;
			}

			CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Result), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
	}
}

float CPattern::GetVariableValue(const std::string& Identifier)
{
	// EnemyPosition.xyz
	// DistanceToEnemy

	if (Identifier == "EnemyPosition.x")
	{
		return m_InternalState.EnemyPosition->m128_f32[0];
	}
	else if (Identifier == "EnemyPosition.y")
	{
		return m_InternalState.EnemyPosition->m128_f32[1];
	}
	else if (Identifier == "EnemyPosition.z")
	{
		return m_InternalState.EnemyPosition->m128_f32[2];
	}
	else if (Identifier == "DistanceToEnemy")
	{
		XMVECTOR Diff{ *m_InternalState.MyPosition - *m_InternalState.EnemyPosition };
		return XMVectorGetX(XMVector3Length(Diff));
	}
	else if (m_umapStackVariableNameToID.find(Identifier) != m_umapStackVariableNameToID.end())
	{
		size_t StackIndex{ m_umapStackVariableNameToID.at(Identifier) };
		return m_Stack[StackIndex];
	}

	return 0;
}
