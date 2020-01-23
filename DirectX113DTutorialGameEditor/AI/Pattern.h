#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "../Core/SharedHeader.h"

class CSyntaxTree;
struct SSyntaxTreeNode;

struct SPatternState
{
	SPatternState() {}
	SPatternState(const XMVECTOR* const _MyPosition) : MyPosition{ _MyPosition } {}

	size_t			StateID{};
	size_t			InstructionIndex{};
	const XMVECTOR* MyPosition{};
	const XMVECTOR* EnemyPosition{};
};

class CPattern
{
public:
	static constexpr size_t KStackSize{ 16 };

public:
	CPattern();
	~CPattern();

public:
	void Load(const char* FileName);

public:
	const SSyntaxTreeNode* Execute(SPatternState& PatternState);

public:
	const std::string& GetFileName() const;
	const std::string& GetFileContent() const;

private:
	bool ExecuteIfNode(const SSyntaxTreeNode* const IfNode);
	void _ExecuteIfNode(SSyntaxTreeNode*& Node);

	void ExecuteFunctionNode(SSyntaxTreeNode*& Node);
	void ExecuteVariableNode(SSyntaxTreeNode*& Node);

	void ExecuteInstructionNode(const SSyntaxTreeNode* const ExecutionNode, size_t& InstructionIndex);

private:
	float GetVariableValue(const std::string& Identifier);

private:
	std::unique_ptr<CSyntaxTree>			m_SyntaxTree{};

private:
	size_t									m_StateCount{};
	std::unordered_map<std::string, size_t>	m_umapStateNameToID{};

private:
	float									m_Stack[KStackSize]{};
	size_t									m_StackCount{};
	std::unordered_map<std::string, size_t>	m_umapStackVariableNameToID{};

private:
	SPatternState							m_InternalState{};

private:
	std::unique_ptr<CSyntaxTree>			m_InstructionSyntaxTree{};

private:
	std::string								m_FileName{};
	std::string								m_FileContent{};
};
