#pragma once
#include <afx.h>
#include <string>
#include <fstream>

namespace atc
{
	class AFX_EXT_CLASS MyLog
	{
	public:
		enum LOG_TYPE {Info, Debug, Warn, Error};						//log type

		MyLog(const std::string& szLogFileName = ".\\log\\defaultLog.log");
		~MyLog();

		MyLog& Write(const CString& szLogStr, LOG_TYPE logType = Info);			//write log to file
		void Enable(bool enable = true);												//enable log

		std::string GetFileName() const;												//get log file path
		void SetFileName(const std::string& szLogFileName = ".\\Log\\defaultLog.log");	//set the file path/name
		
	private:
		std::string m_szLogFileName;		//the path of log file
		std::ofstream m_LogFile;			//log file
		bool m_bEnable;						//enable log

		std::string getDateTime();			//get the date time of now
		void createLogFileDirectory(const std::string& szLogFileName);					//create the directory of log file
	};
}
