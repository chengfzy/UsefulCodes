#include "MyLog.h"
#include <iostream>
#include <sstream>

using namespace std;
using namespace atc;


MyLog::MyLog(const string& szLogFileName)
	: m_bEnable(true)
{
	createLogFileDirectory(szLogFileName);
}


MyLog::~MyLog()
{
}

//set the file path or name
void MyLog::SetFileName(const string& szLogFileName)
{
	createLogFileDirectory(szLogFileName);
}

//write log to file like cout
MyLog& MyLog::Write(const CString& szLogStr, LOG_TYPE logType)
{
	if (m_bEnable)
	{
		CString szWriteStr;
		switch (logType)
		{
		case Info:
			szWriteStr = "INFO";
			break;
		case Debug:
			szWriteStr = "DEBUG";
			break;
		case Warn:
			szWriteStr = "WARN";
			break;
		case Error:
			szWriteStr = "ERROR";
			break;
		default:
			szWriteStr = "UNKNOW";
		}

		szWriteStr.AppendFormat("; %s; %s", getDateTime().c_str(), szLogStr);
		m_LogFile.open(m_szLogFileName, ios::app);
		m_LogFile << szWriteStr << endl;
		m_LogFile.close();
	}
	return *this;
}

//enable log
void MyLog::Enable(bool enable)
{
	m_bEnable = enable;
}

//get file path
string MyLog::GetFileName() const
{
	//string str(m_szLogFileName.GetBuffer());
	string str("LogFile");
	return str;
}

//get the date time of now
string MyLog::getDateTime()
{
	CString dateTimeStr;
	SYSTEMTIME logTime;
	GetLocalTime(&logTime);		//get local time
	dateTimeStr.Format("%04d-%02d-%02d %02d:%02d:%02d.%d", logTime.wYear, logTime.wMonth, logTime.wDay, logTime.wHour, logTime.wMinute, logTime.wSecond, logTime.wMilliseconds);
	return dateTimeStr.GetBuffer();
}

//create directory of the log file
void MyLog::createLogFileDirectory(const string &szLogFileName)
{
	//get the start up module path
	char startPath[1024];
	memset(startPath, 0, sizeof(startPath));	
	::GetModuleFileName(nullptr, startPath, 1024);
	CString szStartPath(startPath);
	szStartPath = szStartPath.Left(szStartPath.ReverseFind('\\'));
	
	//create directory
	CString str = szLogFileName.c_str();
	int end = str.ReverseFind('\\');
	if (-1 != end)
	{
		CString folder = szStartPath + "\\" + str.Left(end);
		CreateDirectory(folder, nullptr);
	}
	
	m_szLogFileName = szStartPath + "\\";
	m_szLogFileName += szLogFileName;		//set the log file name with absoluate path
}
