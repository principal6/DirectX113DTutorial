#include "Tokenizer.h"

#include <fstream>
#include <algorithm>

using std::vector;
using std::string;
using std::ifstream;

CTokenizer::CTokenizer()
{
}

CTokenizer::~CTokenizer()
{
}

void CTokenizer::AddCondition(const SCondition& Condition)
{
	if (std::find(m_vConditions.begin(), m_vConditions.end(), Condition) == m_vConditions.end())
	{
		m_vConditions.emplace_back(Condition);
	}
}

void CTokenizer::AddDivider(const char Divider)
{
	string Div{ Divider };
	if (std::find(m_vDividers.begin(), m_vDividers.end(), Div) == m_vDividers.end())
	{
		m_vDividers.emplace_back(Div);
		m_vDividerFinds.emplace_back();
	}
}

void CTokenizer::AddDivider(const char* Divider)
{
	if (std::find(m_vDividers.begin(), m_vDividers.end(), Divider) == m_vDividers.end())
	{
		m_vDividers.emplace_back(Divider);
		m_vDividerFinds.emplace_back();
	}
}

void CTokenizer::Tokenize(const char* FileName)
{
	m_At = 0;

	if (OpenFile(FileName))
	{
		while (m_At < m_Document.size())
		{
			bool bConditionHandled{ false };
			for (const auto& Condition : m_vConditions)
			{
				size_t Len{ Condition.ConditionString.size() };
				if (CanRead(Len))
				{
					if (Compare(Condition.ConditionString))
					{
						switch (Condition.eToDo)
						{
						case SCondition::EToDo::ReadLine:
							m_vTokens.emplace_back(ReadLine());
							break;
						case SCondition::EToDo::ReadTill:

							break;
						case SCondition::EToDo::SkipLine:
							ReadLine();
							break;
						case SCondition::EToDo::SkipTill:

							break;
						default:
							break;
						}

						bConditionHandled = true;
					}
				}
			}

			if (!bConditionHandled)
			{
				if (!CanRead()) break;

				string Read{ ReadByDivider() };
				if (Read.size())
				{
					m_vTokens.emplace_back(Read);
				}
			}
		}

		if (m_vTokens.size() && (m_vTokens.back().empty() || m_vTokens.back()[0] == '\0'))
		{
			m_vTokens.pop_back();
		}
	}
}

bool CTokenizer::OpenFile(const char* FileName)
{
	ifstream ifs{ FileName };
	if (ifs.is_open())
	{
		ifs.seekg(0, ifs.end);
		auto end_pos{ ifs.tellg() };
		ifs.seekg(0, ifs.beg);

		m_Document.resize(end_pos);
		ifs.read(&m_Document[0], end_pos);
		ifs.close();
		return true;
	}
	return false;
}

void CTokenizer::EraseTokens(const char Cmp)
{
	string CmpString{ Cmp };
	size_t At{};
	while (At < m_vTokens.size())
	{
		if (m_vTokens[At] == CmpString)
		{
			m_vTokens.erase(m_vTokens.begin() + At);
		}
		else
		{
			++At;
		}
	}
}

void CTokenizer::EraseTokens(const char* Cmp)
{
	size_t At{};
	while (At < m_vTokens.size())
	{
		if (m_vTokens[At] == Cmp)
		{
			m_vTokens.erase(m_vTokens.begin() + At);
		}
		else
		{
			++At;
		}
	}
}

const std::vector<std::string>& CTokenizer::GetTokens() const
{
	return m_vTokens;
}

bool CTokenizer::CanRead(size_t Count) const
{
	if (Count == 0) return false;

	return (m_At + Count < m_Document.size());
}

bool CTokenizer::CanReadLine() const
{
	if (CanRead())
	{
		size_t Find{ m_Document.find('\n', m_At) };
		return (Find != string::npos);
	}
	return false;
}

bool CTokenizer::Compare(const std::string& Cmp) const
{
	return (m_Document.substr(m_At, Cmp.size()) == Cmp);
}

std::string CTokenizer::ReadByDivider() const
{
	size_t DividerCount{ m_vDividers.size() };
	size_t StartAt{ m_At };

	while (CanRead())
	{
		for (size_t iDivider = 0; iDivider < DividerCount; ++iDivider)
		{
			const auto& Divider{ m_vDividers[iDivider] };
			if (m_Document.substr(m_At, Divider.size()) == Divider)
			{
				if (m_At == StartAt) m_At += Divider.size();
				string Substring{ m_Document.substr(StartAt, m_At - StartAt) };
				return Substring;
			}
		}
		++m_At;
	}
		
	return "";
}

std::string CTokenizer::ReadLine() const
{
	if (CanReadLine())
	{
		size_t Find{ m_Document.find('\n', m_At) };
		string Substring{ m_Document.substr(m_At, Find - m_At) };
		m_At += (Find - m_At);
		return Substring;
	}
	return "";
}
