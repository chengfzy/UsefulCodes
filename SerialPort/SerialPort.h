#pragma once
#pragma comment(lib, "MyLog.lib")

#include "Port.h"
#include "MyLog.h"
#include <iostream>
#include <string>
#include <vector>

namespace io
{
	class AFX_EXT_CLASS SerialPort: public Port
	{
	public:
		SerialPort(const std::string& szChannelName, const std::string& szLogFileName = "SerialPortLog.log");
		~SerialPort(void);			//deconstructor will implicitly close the port

		//open serial port with setting
		int Open();					//open use asynchronous(overlapped) method
		int Open(bool sync, DWORD baudRate = CBR_38400, BYTE byteSize = 8, BYTE parity = NOPARITY, BYTE stopBits = ONESTOPBIT);
		void Close();				//close serial port

		//getter
		static std::vector<std::string> GetPortList();			//get the serial port list of system
		int GetBaudRate(DWORD& baudRate) const;					//get Baud Rate
		int GetByteSize(BYTE& byteSize) const;					//get Byte Size
		int GetParity(BYTE& parity) const;						//get Parity
		int GetStopBits(BYTE& stopBits) const;					//get Stop Bits
		int GetTimeOuts(COMMTIMEOUTS& timeOuts) const;			//get time out
		//get communication state, or all settings
		int GetPortState(DWORD& baudRate, BYTE& byteSize, BYTE& parity, BYTE& stopBits, COMMTIMEOUTS& timeOuts) const;
		std::string GetLogFileName() const;						//get log file path and name

		//setter
		int SetBaudRate(DWORD baudRate);				//set Baud Rate
		int SetByteSize(BYTE byteSize);					//set Byte Size
		int SetParity(BYTE parity);						//set Parity
		int SetStopBits(BYTE stopBits);					//set Stop Bits
		int SetTimeOuts(const COMMTIMEOUTS timeOuts);	//set tiem out
		int SetPortState(DWORD baudRate, BYTE byteSize, BYTE parity, BYTE stopBits, const COMMTIMEOUTS& timeOuts);	//set communication state
		void SetLogFileName(const std::string& szLogFileName = "SerialPortLog.log");								//set log file path and name
		void EnableLog(bool enable = true);						//enable log

		//write & read
		int Write(const CString& szWriteData, DWORD *dwWrittenLength = nullptr);			//write data to port
		int Read(char* szReadData, DWORD dwMaxReadLength, DWORD *dwReadLength = nullptr);		//read data from port

	private:
		bool m_bSync;				//synchronous(non-overlapped) or asynchronous(overlapped) method
		DWORD m_dwBaudRate;			//baud rate
		BYTE m_nByteSize;			//byte size
		BYTE m_nParity;				//parity
		BYTE m_nStopBits;			//stop bits
		COMMTIMEOUTS m_TimeOuts;	//timeouts

		atc::MyLog m_Log;			//log file	
		CString m_szLogStr;			//log string write to the file
		
		int set();					//set the comm state of serial port
	};
}
