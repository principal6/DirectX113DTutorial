#pragma once

#include <cassert>
#include <vector>
#include <string>

struct SCondition
{
	enum class EToDo
	{
		ReadLine,
		ReadTill,
		SkipLine,
		SkipTill
	};

	SCondition() {}
	SCondition(const std::string& _ConditionString, EToDo _eToDo) :
		ConditionString{ _ConditionString }, eToDo{ _eToDo } 
	{
		assert(_eToDo != EToDo::ReadTill);
	}
	SCondition(const std::string& _ConditionString, EToDo _eToDo, const std::string& _Cmp) :
		ConditionString{ _ConditionString }, eToDo{ _eToDo }, Cmp{ _Cmp } 
	{
		assert(_eToDo == EToDo::ReadTill);
	}

	bool operator==(const SCondition& b)
	{
		return (ConditionString == b.ConditionString);
	}

	std::string	ConditionString{};
	EToDo		eToDo{};
	std::string	Cmp{};
};

class CTokenizer
{
public:
	CTokenizer();
	~CTokenizer();

public:
	void AddCondition(const SCondition& Condition);
	void AddDivider(const char Divider);
	void AddDivider(const char* Divider);

public:
	void Tokenize(const char* FileName);

private:
	bool OpenFile(const char* FileName);

public:
	void EraseTokens(const char Cmp);
	void EraseTokens(const char* Cmp);

public:
	const std::vector<std::string>& GetTokens() const;

private:
	bool CanRead(size_t Count = 1) const;
	bool CanReadLine() const;
	bool Compare(const std::string& Cmp) const;

private:
	std::string ReadByDivider() const;
	std::string ReadLine() const;

private:
	std::string							m_Document{};
	mutable std::string					m_Peeked{};
	mutable size_t						m_At{};

private:
	std::vector<SCondition>				m_vConditions{};
	std::vector<std::string>			m_vDividers{};
	mutable std::vector<std::size_t>	m_vDividerFinds{};

private:
	std::vector<std::string>			m_vTokens{};
};