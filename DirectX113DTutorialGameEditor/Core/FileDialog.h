#pragma once

#include "SharedHeader.h"

class CFileDialog final
{
public:
	CFileDialog(const char* const WorkingDirectory) : m_WorkingDirectory{ WorkingDirectory } {}
	~CFileDialog() {}

public:
	bool OpenFileDialog(const char* const Filter, const char* const Title);
	bool SaveFileDialog(const char* const Filter, const char* const Title, const char* const DefaultExtension);

public:
	const std::string& GetFileName() const { return m_FileName; }
	const std::string& GetFileNameWithoutPath() const { return m_FileNameWithoutPath; }
	const std::string& GetFileNameWithoutExt() const { return m_FileNameWithoutExt; }
	const std::string& GetDirectory() const { return m_Directory; }
	const std::string& GetRelativeFileName() const { return m_RelativeFileName; }
	const std::string& GetCapitalExtension() const { return m_CapitalExtension; }

private:
	const char* const	m_WorkingDirectory{};

private:
	std::string			m_FileName{};
	std::string			m_FileNameWithoutPath{};
	std::string			m_FileNameWithoutExt{};
	std::string			m_Directory{};
	std::string			m_RelativeFileName{};
	std::string			m_CapitalExtension{};
	OPENFILENAME		m_OpenFileName{};
};
