#pragma once

#include <memory>
#include <vector>
#include <string>

struct SSyntaxTreeNode;
class CSyntaxTree;

struct SDeepestNode
{
	SSyntaxTreeNode*	Node{};
	size_t				Depth{};
};

class CAnalyzer
{
public:
	CAnalyzer();
	~CAnalyzer();

public:
	void Analyze(const std::vector<std::string>& vTokens);
	const std::string& Serialize();
	SSyntaxTreeNode*& GetRootNode();

public:
	void AddDirective(const std::string& Directive);
	void AddOperator(const std::string& Operator);
	void AddLiteral(const std::string& Literal);

private:
	bool IsKeyword(size_t Offset = 0) const;
	bool IsDirective(size_t Offset = 0) const;
	bool IsOperator(size_t Offset = 0) const;
	bool IsLiteral(size_t Offset = 0) const;

private:
	void BuildSyntaxTree();

	void MakeGroupingNodes(SSyntaxTreeNode*& CurrentNode, const std::string& Open, const std::string& Close);
	void RemoveUnnecessaryGroupingNodes(SSyntaxTreeNode*& CurrentNode);
	void OrganizeNonCastingGroupingNodes(SSyntaxTreeNode*& CurrentNode);
	void OrganizeFunctionNodes(SSyntaxTreeNode*& CurrentNode);
	void OrganizeCastingNodes(SSyntaxTreeNode*& CurrentNode);
	void OrganizeUnaryOperatorNodes(SSyntaxTreeNode*& CurrentNode, const std::string& UnaryOperator);
	void OrganizeBinaryOperatorNodes(SSyntaxTreeNode*& CurrentNode, const std::string& BinaryOperator);

	void OrganizeAssignmentNodes(SSyntaxTreeNode*& CurrentNode);
	void OrganizeDeclarationNodes(SSyntaxTreeNode*& CurrentNode, const std::string& TypeName);
	void RemovePunctuator(SSyntaxTreeNode*& CurrentNode, const std::string& Punctuator);

private:
	bool CanRead(size_t Count = 1, size_t Offset = 0) const;
	bool Compare(const std::string Cmp, size_t Offset = 0) const;
	const std::string& GetToken(size_t Offset = 0) const;
	void Skip(size_t Count = 1) const;

private:
	std::vector<std::string>		m_vDirectives{};
	std::vector<std::string>		m_vOperators{};
	std::vector<std::string>		m_vLiterals{};

private:
	mutable size_t					m_TokenAt{};
	std::vector<std::string>		m_vTokens{};

private:
	std::unique_ptr<CSyntaxTree>	m_SyntaxTree{};
};
