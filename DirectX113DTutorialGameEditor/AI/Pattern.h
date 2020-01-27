#pragma once

#include "../Core/SharedHeader.h"
#include "PatternTypes.h"

class CSyntaxTree;
struct SSyntaxTreeNode;

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
	void ExecuteNonFunctionNode(SSyntaxTreeNode*& Node);

	void ExecuteInstructionNode(const SSyntaxTreeNode* const ExecutionNode);

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
	SPatternState							m_CopiedState{};

private:
	std::unique_ptr<CSyntaxTree>			m_InstructionSyntaxTree{};

private:
	std::string								m_FileName{};
	std::string								m_FileContent{};
};
