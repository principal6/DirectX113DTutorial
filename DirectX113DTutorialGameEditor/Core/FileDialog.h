#pragma once

#include "SharedHeader.h"

class CFileDialog final
{
public:
	CFileDialog(const char* const WorkingDirectory) : m_WorkingDirectory{ WorkingDirectory } {}
	~CFileDialog() {}

	bool OpenFileDialog(const char* const Filter, const char* const Title);
	bool SaveFileDialog(const char* const Filter, const char* const Title, const char* const DefaultExtension);

	const std::string& GetFileName() const { return m_FileName; }
	const std::string& GetFileNameWithoutPath() const { return m_FileNameWithoutPath; }
	const std::string& GetRelativeFileName() const { return m_RelativeFileName; }

private:
	const char* const	m_WorkingDirectory{};

private:
	std::string			m_FileName{};
	std::string			m_FileNameWithoutPath{};
	std::string			m_RelativeFileName{};
	OPENFILENAME		m_OpenFileName{};
};
