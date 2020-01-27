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
using std::min;

CPattern::CPattern()
{
}

CPattern::~CPattern()
{
}

void CPattern::Load(const char* FileName)
{
	m_FileName = FileName;

	ifstream ifs{ FileName };
	if (ifs.is_open())
	{
		ifs.seekg(0, ifs.end);
		auto end_pos{ ifs.tellg() };
		ifs.seekg(0, ifs.beg);

		m_FileContent.resize(end_pos);
		ifs.read(&m_FileContent[0], end_pos);
		ifs.close();
	}

	srand((unsigned int)time(nullptr));

	CTokenizer Tokenizer{};
	CAnalyzer Analyzer{};

	{
		Tokenizer.AddCondition(SCondition("//", SCondition::EToDo::SkipLine));
		Tokenizer.AddDivider(' ');
		Tokenizer.AddDivider('\t');
		Tokenizer.AddDivider('\n');

		Tokenizer.AddDivider('\'');
		Tokenizer.AddDivider('\"');
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
		Analyzer.AddOperator("\'");
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
			m_umapStateNameToID[StateNode->vChildNodes[0]->Identifier] = m_StateCount;
			++m_StateCount;
		}
	}
}

const SSyntaxTreeNode* CPattern::Execute(SPatternState& PatternState)
{
	if (!m_SyntaxTree) return nullptr;
	if (!m_SyntaxTree->GetRootNode()) return nullptr;

	m_CopiedState = PatternState;

	const auto& StateNode{ m_SyntaxTree->GetRootNode()->vChildNodes[PatternState.StateID] };
	const auto& GroupingNode{ StateNode->vChildNodes.back() };

	size_t SubStateNodeCount{ GroupingNode->vChildNodes.size() };
	for (size_t iSubStateNode = 0; iSubStateNode < SubStateNodeCount; ++iSubStateNode)
	{
		auto& SubStateNode{ GroupingNode->vChildNodes[iSubStateNode] };

		ExecuteFunctionNode(SubStateNode);

		if (SubStateNode->Identifier == "else" && iSubStateNode == SubStateNodeCount - 1) // last else
		{
			ExecuteInstructionNode(SubStateNode->vChildNodes.back());

			break;
		}
		if (SubStateNode->Identifier == "if") // if, else if
		{
			// process if
			bool IfResult{ ExecuteIfNode(SubStateNode) }; 
			if (IfResult)
			{
				ExecuteInstructionNode(SubStateNode->vChildNodes.back());
			}

			// process else if
			if (iSubStateNode + 1 < SubStateNodeCount)
			{
				auto& NextControlNode{ GroupingNode->vChildNodes[iSubStateNode + 1] };
				if (NextControlNode->Identifier == "else")
				{
					if (IfResult == true) break;
				}
			}
		}
	}

	PatternState = m_CopiedState;

	return m_InstructionSyntaxTree->GetRootNode();
}

const std::string& CPattern::GetFileName() const
{
	return m_FileName;
}

const std::string& CPattern::GetFileContent() const
{
	return m_FileContent;
}

bool CPattern::ExecuteIfNode(const SSyntaxTreeNode* const IfNode)
{
	if (!IfNode) return false;
	if (IfNode->vChildNodes.empty()) return false;

	CSyntaxTree Tree{};
	Tree.CopyFrom(IfNode);

	auto& OperatorNode{ Tree.GetRootNode()->vChildNodes[0] };
	_ExecuteIfNode(OperatorNode);

	return (OperatorNode->Identifier == "true" ? true : false);
}

void CPattern::_ExecuteIfNode(SSyntaxTreeNode*& Node)
{
	if (!Node) return;

	if (Node->eType == SSyntaxTreeNode::EType::Operator)
	{
		for (auto& ChildNode : Node->vChildNodes)
		{
			_ExecuteIfNode(ChildNode);
		}

		if (Node->Identifier == "!")
		{
			// unary

			bool bChild{ (Node->vChildNodes[0]->Identifier == "true" ? true : false) };

			CSyntaxTree::Substitute(SSyntaxTreeNode((bChild == true ? "false" : "true"), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
		else
		{
			// binary

			const auto& Left{ Node->vChildNodes[0]->Identifier };
			const auto& Right{ Node->vChildNodes[1]->Identifier };

			bool Result{ false };
			if (Node->Identifier == "==")
			{
				Result = (Left == Right);
			}
			else if (Node->Identifier == "!=")
			{
				Result = (Left != Right);
			}
			else if (Node->Identifier == ">=")
			{
				float fLeft{ stof(Left) };
				float fRight{ stof(Right) };
				Result = (fLeft >= fRight);
			}
			else if (Node->Identifier == ">")
			{
				float fLeft{ stof(Left) };
				float fRight{ stof(Right) };
				Result = (fLeft > fRight);
			}
			else if (Node->Identifier == "<=")
			{
				float fLeft{ stof(Left) };
				float fRight{ stof(Right) };
				Result = (fLeft <= fRight);
			}
			else if (Node->Identifier == "<")
			{
				float fLeft{ stof(Left) };
				float fRight{ stof(Right) };
				Result = (fLeft < fRight);
			}
			else if (Node->Identifier == "&&")
			{
				bool bLeft{ (Left == "true" ? true : false) };
				bool bRight{ (Right == "true" ? true : false) };
				Result = (bLeft && bRight);
			}
			else if (Node->Identifier == "||")
			{
				bool bLeft{ (Left == "true" ? true : false) };
				bool bRight{ (Right == "true" ? true : false) };
				Result = (bLeft || bRight);
			}

			CSyntaxTree::Substitute(SSyntaxTreeNode((Result == true ? "true" : "false"), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
		}
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
	if (Node->eType != SSyntaxTreeNode::EType::Identifier) return;

	for (auto& Argument : Node->vChildNodes)
	{
		if (Argument->vChildNodes.size())
		{
			// function node
			ExecuteFunctionNode(Argument);
		}
		else
		{
			// variable or literal node
			ExecuteNonFunctionNode(Argument);
		}
	}

	if (Node->Identifier == "random")
	{
		assert(Node->vChildNodes.size() == 2);

		ExecuteNonFunctionNode(Node->vChildNodes[0]);
		ExecuteNonFunctionNode(Node->vChildNodes[1]);

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
		assert(Node->vChildNodes.size() == 1);

		m_CopiedState.StateID = m_umapStateNameToID.at(Node->vChildNodes[0]->Identifier);
	}
	else if (Node->Identifier == "set_value")
	{
		assert(Node->vChildNodes.size() == 2);

		if (Node->vChildNodes[0]->Identifier == "WalkSpeed")
		{
			ExecuteNonFunctionNode(Node->vChildNodes[1]);
			float Value{ stof(Node->vChildNodes[1]->Identifier) };

			m_CopiedState.WalkSpeed = Value;
		}
	}
}

void CPattern::ExecuteInstructionNode(const SSyntaxTreeNode* const ExecutionNode)
{
	if (!ExecutionNode) return;
	if (ExecutionNode->vChildNodes.empty()) return;

	m_CopiedState.InstructionIndex = min(m_CopiedState.InstructionIndex, ExecutionNode->vChildNodes.size() - 1);
	if (m_CopiedState.InstructionIndex == 0)
	{
		m_StackCount = 0; // @important
		m_umapStackVariableNameToID.clear();
	}

	const auto& CurrentNode{ ExecutionNode->vChildNodes[m_CopiedState.InstructionIndex] };
	const auto& FirstChildNode{ CurrentNode->vChildNodes.front() };
	if (FirstChildNode->Identifier == "=" ||
		FirstChildNode->Identifier == "+=" ||
		FirstChildNode->Identifier == "-=" ||
		FirstChildNode->Identifier == "*=" ||
		FirstChildNode->Identifier == "/=")
	{
		// variable

		m_InstructionSyntaxTree->CopyFrom(CurrentNode);
		ExecuteNonFunctionNode(m_InstructionSyntaxTree->GetRootNode());

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

	++m_CopiedState.InstructionIndex;
	if (m_CopiedState.InstructionIndex >= ExecutionNode->vChildNodes.size()) m_CopiedState.InstructionIndex = 0;
}

void CPattern::ExecuteNonFunctionNode(SSyntaxTreeNode*& Node)
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
				ExecuteNonFunctionNode(Node->vChildNodes[0]);
			}

			if (Node->vChildNodes[0]->Identifier == "true" ||
				Node->vChildNodes[0]->Identifier == "false")
			{
				if (Node->Identifier == "!")
				{
					string Result{ (Node->vChildNodes[0]->Identifier == "true") ? "false" : "true" };
					
					CSyntaxTree::Substitute(SSyntaxTreeNode(Result, SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
				}
			}
			else
			{
				float Result{ stof(Node->vChildNodes[0]->Identifier) };
				if (Node->Identifier == "-")
				{
					Result = -Result;
				}

				CSyntaxTree::Substitute(SSyntaxTreeNode(to_string(Result), SSyntaxTreeNode::EType::Literal, Node->ParentNode), Node);
			}
		}
		else if (ChildCount == 2)
		{
			// binary
			if (Node->vChildNodes[0]->eType != SSyntaxTreeNode::EType::Literal)
			{
				ExecuteNonFunctionNode(Node->vChildNodes[0]);
			}
			if (Node->vChildNodes[1]->eType != SSyntaxTreeNode::EType::Literal)
			{
				ExecuteNonFunctionNode(Node->vChildNodes[1]);
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
		return m_CopiedState.EnemyPosition->m128_f32[0];
	}
	else if (Identifier == "EnemyPosition.y")
	{
		return m_CopiedState.EnemyPosition->m128_f32[1];
	}
	else if (Identifier == "EnemyPosition.z")
	{
		return m_CopiedState.EnemyPosition->m128_f32[2];
	}
	else if (Identifier == "DistanceToEnemy")
	{
		XMVECTOR Diff{ *m_CopiedState.MyPosition - *m_CopiedState.EnemyPosition };
		return XMVectorGetX(XMVector3Length(Diff));
	}
	else if (m_umapStackVariableNameToID.find(Identifier) != m_umapStackVariableNameToID.end())
	{
		size_t StackIndex{ m_umapStackVariableNameToID.at(Identifier) };
		return m_Stack[StackIndex];
	}

	return 0;
}
