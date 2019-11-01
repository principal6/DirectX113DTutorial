#pragma once

#include "SharedHeader.h"

class CFileDialog final
{
public:
	CFileDialog(const char* const WorkingDirectory) : m_WorkingDirectory{ WorkingDirectory } {}
	~CFileDialog() {}

	bool OpenFileDialog(const char* const Filter, const char* const Title);
	bool SaveFileDialog(const char* const Filter, const char* const Title, const char* const DefaultExtension);

	const string& GetFileName() const { return m_FileName; }
	const string& GetFileNameWithoutPath() const { return m_FileNameWithoutPath; }
	const string& GetRelativeFileName() const { return m_RelativeFileName; }

private:
	const char* const	m_WorkingDirectory{};

private:
	string				m_FileName{};
	string				m_FileNameWithoutPath{};
	string				m_RelativeFileName{};
	OPENFILENAME		m_OpenFileName{};
};
