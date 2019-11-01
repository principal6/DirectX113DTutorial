#include "FileDialog.h"

bool CFileDialog::OpenFileDialog(const char* const Filter, const char* const Title)
{
	char FileName[MAX_PATH]{};
	m_OpenFileName.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
	m_OpenFileName.lpstrDefExt = nullptr;
	m_OpenFileName.lpstrFilter = Filter;
	m_OpenFileName.lpstrFile = FileName;
	m_OpenFileName.lpstrTitle = Title;
	m_OpenFileName.lStructSize = sizeof(OPENFILENAME);
	m_OpenFileName.nMaxFile = MAX_PATH;

	BOOL Result{ GetOpenFileName(&m_OpenFileName) };

	m_FileNameWithoutPath = m_FileName = FileName;
	if (m_FileNameWithoutPath.size())
	{
		size_t Found{ m_FileNameWithoutPath.find_last_of('\\') };
		m_FileNameWithoutPath = m_FileNameWithoutPath.substr(Found + 1);
	}

	if (m_FileName.size() > strlen(m_WorkingDirectory))
	{
		m_RelativeFileName = m_FileName.substr(strlen(m_WorkingDirectory) + 1);
	}

	SetCurrentDirectoryA(m_WorkingDirectory);

	return Result;
}

bool CFileDialog::SaveFileDialog(const char* const Filter, const char* const Title, const char* const DefaultExtension)
{
	char FileName[MAX_PATH]{};
	m_OpenFileName.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
	m_OpenFileName.lpstrDefExt = DefaultExtension;
	m_OpenFileName.lpstrFilter = Filter;
	m_OpenFileName.lpstrFile = FileName;
	m_OpenFileName.lpstrTitle = Title;
	m_OpenFileName.lStructSize = sizeof(OPENFILENAME);
	m_OpenFileName.nMaxFile = MAX_PATH;

	BOOL Result{ GetSaveFileName(&m_OpenFileName) };

	m_FileNameWithoutPath = m_FileName = FileName;
	if (m_FileNameWithoutPath.size())
	{
		size_t Found{ m_FileNameWithoutPath.find_last_of('\\') };
		m_FileNameWithoutPath = m_FileNameWithoutPath.substr(Found + 1);
	}

	if (m_FileName.size() > strlen(m_WorkingDirectory))
	{
		m_RelativeFileName = m_FileName.substr(strlen(m_WorkingDirectory) + 1);
	}

	SetCurrentDirectoryA(m_WorkingDirectory);

	return Result;
}