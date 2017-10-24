#pragma once
#include <afx.h>
#include <iostream>
#include <string>

namespace io
{
	class AFX_EXT_CLASS Port
	{
	public:
		//Error Code Enum
		enum ErrorCode
		{		
			SUCCESS = 0,
			//port open error
			ERROR_PORT_ALREADY_OPEN = 01,
			ERROR_PORT_NOT_EXIST = 02,
			ERROR_PORT_IS_IN_USED = 03,
			ERROR_OPEN_PORT_FAILED = 04,
			ERROR_PORT_NOT_OPEN = 05,
			//write, read error
			ERROR_WRITE_PORT_FAILED = 10,
			ERROR_WRITE_NOT_FINISH = 11,		//write data not finished cause of time-out or something
			ERROR_READ_PORT_FAILED = 12,
			WARNING_READ_DATA_EXCEED = 13,		//the input szReadData lenthg is less than the data in quene
			ERROR_READ_PORT_TIMEOUT = 14,		//read data not finished because of time out
			//setting error
			ERROR_GET_COMM_STATE_FALIED = 20,
			ERROR_PORT_GETTING_FALIED = 21,
			ERROR_PORT_SETTING_FAILED = 22,
			ERROR_INVALID_SET_PARAMETER = 23
		};

		Port(const std::string& szChannelName);
		virtual ~Port(void);
		//virutal function should overide
		virtual int Open() = 0;				//open port
		virtual void Close() = 0;										//close port
		virtual bool IsOpen() const;									//whether the port is open
		virtual int Write(const CString& szWriteData, DWORD* dwWrittenLength = nullptr) = 0;				//write data to port
		virtual int Read(char* szReadData, DWORD dwMaxReadLength, DWORD* dwReadLength = nullptr) = 0;			//read data from port

		//Getter
		HANDLE GetPortHandle();		 // Get the port handle, for some more setting if user need

		//Log
		virtual std::string GetLogFileName() const = 0;						//get log file path and name
		virtual void SetLogFileName(const std::string& szLogFileName) = 0;	//set log file path and name
		virtual void EnableLog(bool enable) = 0;						//enable log

	protected:
		HANDLE m_hPort;				// the port handle
		std::string m_szChannelName;	//the channel name of port
	};
}
