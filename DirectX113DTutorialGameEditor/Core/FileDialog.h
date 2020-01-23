#pragma once

#include "SharedHeader.h"

class CFileDialog final
{
public:
	CFileDialog() {}
	CFileDialog(const std::string& InitialDirectory);
	~CFileDialog() {}

public:
	// 예시
	// ("모델 파일(*.fbx)\0*.fbx\0", "초목 오브젝트 불러오기")
	bool OpenFileDialog(const char* const Filter, const char* const Title);

	// 예시
	// ("지형 파일(*.terr)\0*.terr\0", "지형 파일 내보내기", ".terr")
	bool SaveFileDialog(const char* const Filter, const char* const Title, const char* const DefaultExtension);

public:
	const std::string& GetFileName() const { return m_FileName; }
	const std::string& GetFileNameWithoutPath() const { return m_FileNameWithoutPath; }
	const std::string& GetFileNameWithoutExt() const { return m_FileNameWithoutExt; }
	const std::string& GetDirectory() const { return m_Directory; }
	const std::string& GetRelativeFileName() const { return m_RelativeFileName; }
	const std::string& GetCapitalExtension() const { return m_CapitalExtension; }

private:
	std::string			m_FileName{};
	std::string			m_FileNameWithoutPath{};
	std::string			m_FileNameWithoutExt{};
	std::string			m_Directory{};
	std::string			m_RelativeFileName{};
	std::string			m_CapitalExtension{};
	OPENFILENAME		m_OpenFileName{};

private:
	std::string			m_InitialDirectory{};
};
