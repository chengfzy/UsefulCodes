#include "SerialPort.h"
#include <algorithm>

using namespace std;
using namespace atc;
using namespace io;

SerialPort::SerialPort(const string& szChannelName, const string& szLogFileName)
	: Port(szChannelName), m_dwBaudRate(CBR_38400), m_nByteSize(8), m_nParity(NOPARITY), m_nStopBits(ONESTOPBIT)
	, m_Log(szLogFileName)
{
	//baud rate = 38.4k, communication state is 8-1-N: data size = 8, stop bits = 1, and no parity	
	//default time outs
	memset(&m_TimeOuts, 0, sizeof(m_TimeOuts));
	
	// [MAXDWORD, 0, 0]: return buffer immediately
	//m_TimeOuts.ReadIntervalTimeout = 1000;
	//m_TimeOuts.ReadTotalTimeoutMultiplier = 500;
	//m_TimeOuts.ReadTotalTimeoutConstant = 5000;
	//m_TimeOuts.WriteTotalTimeoutMultiplier = 500;	//have no write time out
	//m_TimeOuts.WriteTotalTimeoutConstant = 5000;
	m_TimeOuts.ReadIntervalTimeout = 10;
	m_TimeOuts.ReadTotalTimeoutMultiplier = 10;
	m_TimeOuts.ReadTotalTimeoutConstant = 5000;
	m_TimeOuts.WriteTotalTimeoutMultiplier = 10;	//have no write time out
	m_TimeOuts.WriteTotalTimeoutConstant = 5000;
}


SerialPort::~SerialPort(void)
{
	Close();
}

/// <summary>
/// default and overide Open() function, use asynchronous(overlapped) communication method
/// </summary>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::Open()
{	
	return Open(false);
}

/// <summary>
/// open serial port with setting
/// </summary>
/// <param name="sync">communication method, true: synchronous(non-overlapped); false: asynchronous(overlapped)</param>
/// <param name="baudRate">baud rate		</param>
/// <param name="byteSize">byte size		</param>
/// <param name="parity">parity		</param>
/// <param name="stopBits">stop bits		</param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::Open(bool sync, DWORD baudRate, BYTE byteSize, BYTE parity, BYTE stopBits)
{
	//serial port already exist
	if (nullptr != m_hPort)
	{
		m_szLogStr.Format("Error %d; Port already open", ERROR_PORT_ALREADY_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_ALREADY_OPEN;
	}
		
	m_bSync = sync;
	CString szLog;
	szLog.Format("Openning serial port %s in synchronous communication", m_szChannelName.c_str());
	m_Log.Write(szLog);

	// open com using synchronous/non-overlapped communication
	m_hPort = CreateFile(("\\\\.\\" + m_szChannelName).c_str(),		//note the COM10+ port in windows
					  GENERIC_READ | GENERIC_WRITE,	//allow read and write
					  0,		//FILE_SHARE_READ | FILE_SHARE_WRITE,							// share mode
					  nullptr,
					  OPEN_EXISTING,				//Must be OPEN_EXISTING for COM					  
					  sync? 0: (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED),	//synchronous(non-overlapped) or asynchronous(overlapped) commucation
					  nullptr);						// Must be nullptr for COM
	//open fail, and error handling
	if (INVALID_HANDLE_VALUE == m_hPort)
	{
		m_hPort = nullptr;				//reset to nullptr if error
		switch (GetLastError())
		{
		case ERROR_FILE_NOT_FOUND:
			m_szLogStr.Format("Error %d; port not exists", ERROR_PORT_NOT_EXIST);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_PORT_NOT_EXIST;
			break;
		case ERROR_ACCESS_DENIED:
			m_szLogStr.Format("Error %d; port is in used", ERROR_PORT_IS_IN_USED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_PORT_IS_IN_USED;
			break;
		default:
			m_szLogStr.Format("Error %d; Open port failed", ERROR_OPEN_PORT_FAILED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_OPEN_PORT_FAILED;
			break;
		}
	}

	//set the communication input/output buffer size, use default if not set
	//::SetupComm(m_hPort, 4096, 4096);

	//set mask
	BOOL bStatus = SetCommMask(m_hPort, EV_RXCHAR | EV_RXFLAG);
	if (!bStatus)
	{
		m_szLogStr.Format("Error %d; error in setting mask", ERROR_PORT_SETTING_FAILED);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_SETTING_FAILED;
	}

	//clear the input/output buffer
	bStatus = PurgeComm(m_hPort, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);
	if (!bStatus)
	{
		m_szLogStr.Format("Error %d; error in purge comm", ERROR_PORT_SETTING_FAILED);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_SETTING_FAILED;
	}
	
	//set the default communication state to the COM port
	m_dwBaudRate = baudRate;
	m_nByteSize = byteSize;
	m_nParity = parity;
	m_nStopBits = stopBits;
		
	//set communication state
	int nStatus = set();
	if (SUCCESS != nStatus)
	{
		return nStatus;
	}
	else
	{
		//set communication timeouts
		return SetTimeOuts(m_TimeOuts);
	}
}

/// <summary>
/// close serial port
/// </summary>
void SerialPort::Close()
{
	if (nullptr != m_hPort)
	{
		CloseHandle(m_hPort);
		m_hPort = nullptr;
		m_Log.Write("Close serial port");
	}
}

/// <summary>
/// get the serial port list of system
/// </summary>
/// <returns>
/// the set of serial port name
/// </returns>
std::vector<std::string> SerialPort::GetPortList()
{
	vector<string> aszPortName;
	//open the reg of serial port	
	int nStatus(0);
	HKEY hKey;			//key handle
	nStatus = ::RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM\\", &hKey);
	//read the serial port name
	DWORD index(0);
	const int maxLen = 255;
	CHAR keyName[maxLen];
	UCHAR keyData[maxLen];	
	do
	{
		DWORD nameLen = maxLen;
		DWORD dataLen = maxLen;
		nStatus = ::RegEnumValue(hKey, index++, keyName, &nameLen, nullptr, nullptr, keyData, &dataLen);
		if (ERROR_SUCCESS == nStatus)
		{
			CString szPortName;
			szPortName.Format("%s", keyData);
			aszPortName.push_back(szPortName.GetBuffer());
		}
	} while (ERROR_SUCCESS == nStatus || ERROR_MORE_DATA == nStatus);
	::RegCloseKey(hKey);	//close handle

	//sort the port list
	//sort(aszPortName.begin(), aszPortName.end());

	return aszPortName;
}

/// <summary>
/// get baud rate
/// </summary>
/// <param name="baudRate"> the output baudRate </param>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return ERROR Code
/// </returns>
int SerialPort::GetBaudRate(DWORD& baudRate) const
{
	if (IsOpen())
	{		
		baudRate = m_dwBaudRate;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// get byte size
/// </summary>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return ERROR Code
/// </returns>
int SerialPort::GetByteSize(BYTE& byteSize) const
{
	if (IsOpen())
	{
		byteSize = m_nByteSize;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// get parity
/// </summary>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return ERROR Code
/// </returns>
int  SerialPort::GetParity(BYTE& parity) const
{	
	if (IsOpen())
	{
		parity = m_nParity;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// get stopbits
/// </summary>
/// <param name="stopBits"> output stop bits </param>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return ERROR Code
/// </returns>
int SerialPort::GetStopBits(BYTE& stopBits) const
{
	if (IsOpen())
	{
		stopBits = m_nStopBits;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// get time out
/// </summary>
/// <param name="timeOuts"> output time out struct	</param>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return ERROR Code
/// </returns>
int SerialPort::GetTimeOuts(COMMTIMEOUTS& timeOuts) const
{
	if (IsOpen())
	{
		timeOuts = m_TimeOuts;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// Get the communication state, or get the setting for the serial port communication
/// </summary>
/// <param name="baudRate"> output, baud rate </param>
/// <param name="byteSize"> output, byte size </param>
/// <param name="parity"> output, parity </param>
/// <param name="stopBits"> output, stopbits </param>
/// <param name="timeOuts"> output, timeouts struct </param>
/// <returns>
/// return SUCCESS if get state correctly, otherwise return Error
/// </returns>
int SerialPort::GetPortState(DWORD& baudRate, BYTE& byteSize, BYTE& parity, BYTE& stopBits, COMMTIMEOUTS& timeOuts) const
{
	//get communication state, or all settings
	if (IsOpen())
	{
		baudRate = m_dwBaudRate;
		byteSize = m_nByteSize;
		parity = m_nParity;
		stopBits = m_nStopBits;
		timeOuts = m_TimeOuts;
		return SUCCESS;
	}
	else
		return ERROR_PORT_NOT_OPEN;
}

/// <summary>
/// get log file path and name
/// </summary>
/// <returns>
/// log file path and name
/// </returns>
string SerialPort::GetLogFileName() const
{
	return m_Log.GetFileName();
}

/// <summary>
/// set baud rate
/// </summary>
/// <param name="baudRate">	baud rate </param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetBaudRate(const DWORD baudRate)
{	
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; error in set baud rate, port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	DWORD oldBaudRate = m_dwBaudRate;
	m_dwBaudRate = baudRate;		//change to new baud rate
	int nStatus = set();

	//restore the old setting if set() ERROR
	if (SUCCESS != nStatus)
	{
		m_dwBaudRate = oldBaudRate;
	}
	
	return nStatus;
}

/// <summary>
/// set byte size
/// </summary>
/// <param name="byteSize">	byte size </param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetByteSize(const BYTE byteSize)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; error in set byte size, port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	BYTE oldByteSize = m_nByteSize;
	m_nByteSize = byteSize;
	int nStatus = set();
	if (SUCCESS != nStatus)		//restore the old setting if set() ERROR
		m_nByteSize = oldByteSize;
	
	return nStatus;
}

/// <summary>
/// Set Stop Bits
/// </summary>
/// <param name="stopBits">	stop bits </param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetStopBits(const BYTE stopBits)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; error in set stop bits, port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	BYTE oldStopBits = m_nStopBits;
	m_nStopBits = stopBits;
	int nStatus = set();
	if (SUCCESS != nStatus)		//restore the old setting if set() ERROR
		m_nStopBits = oldStopBits;

	return nStatus;
}

/// <summary>
/// set parity
/// </summary>
/// <param name="byteSize">	parity	</param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetParity(const BYTE parity)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; error in set stop bits, port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	BYTE oldParity = m_nParity;
	m_nParity = parity;
	int nStatus = set();
	if (SUCCESS != nStatus)	//restore the old setting if set() ERROR
		m_nParity = oldParity;

	return nStatus;
}

/// <summary>
/// set time out 
/// </summary>
/// <param name="timeOuts"> timeout struct </param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetTimeOuts(const COMMTIMEOUTS timeOuts)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; Port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	//set timeouts
	BOOL bStatus = SetCommTimeouts(m_hPort, const_cast<LPCOMMTIMEOUTS>(&timeOuts));
	if (!bStatus)
	{
		m_szLogStr.Format("Error %d; Failed to set communication timeouts", ERROR_PORT_SETTING_FAILED);
		m_Log.Write(m_szLogStr);
		return ERROR_PORT_SETTING_FAILED;
	}
	else
	{
		m_TimeOuts = timeOuts;		//save the timeouts if success
		m_szLogStr.Format("Set communication timeouts = [%d %d %d %d %d]",
			m_TimeOuts.ReadIntervalTimeout, m_TimeOuts.ReadTotalTimeoutConstant, m_TimeOuts.ReadTotalTimeoutMultiplier,
			m_TimeOuts.WriteTotalTimeoutConstant, m_TimeOuts.WriteTotalTimeoutMultiplier);
		m_Log.Write(m_szLogStr);
		return SUCCESS;
	}
}

/// <summary>
/// set the communication state
/// </summary>
/// <param name="baudRate"> baud rate </param>
/// <param name="byteSize"> byte size </param>
/// <param name="parity"> parity </param>
/// <param name="stopBits"> stop bits </param>
/// <param name="timeOuts"> timeouts</param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::SetPortState(DWORD baudRate, BYTE byteSize, BYTE parity, BYTE stopBits, const COMMTIMEOUTS& timeOuts)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; error in set port state, port is not open", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}
	//save the old setting
	DWORD oldBaudRate = m_dwBaudRate;
	BYTE oldByteSize = m_nByteSize;
	BYTE oldParity = m_nParity;
	BYTE oldStopBits = m_nStopBits;
	COMMTIMEOUTS oldTimeOuts = m_TimeOuts;
	//change to new setting
	m_dwBaudRate = baudRate;
	m_nByteSize = byteSize;
	m_nParity = parity;
	m_nStopBits = stopBits;
	m_TimeOuts = timeOuts;

	int nStatus = set();
	//restore the old setting if set() ERROR
	if (SUCCESS != nStatus)
	{
		m_dwBaudRate = oldBaudRate;
		m_nByteSize = oldByteSize;
		m_nParity = oldParity;
		m_nStopBits = oldStopBits;
		return ERROR_PORT_SETTING_FAILED;
	}
	else
	{

		//set communication state
		BOOL bStatus = SetCommTimeouts(m_hPort, const_cast<LPCOMMTIMEOUTS>(&timeOuts));
		if (!bStatus)
		{
			m_szLogStr.Format("Error %d; error in set port state", ERROR_PORT_SETTING_FAILED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_PORT_SETTING_FAILED;
		}
		else
		{
			m_TimeOuts = timeOuts;		//save the timeouts if success
			return SUCCESS;
		}
	}	
}

/// <summary>
/// set log file path and name
/// </summary>
/// <param name="logFileName"> log file path and name </param>
void SerialPort::SetLogFileName(const string& szLogFileName)
{
	CString szLog;
	szLog.Format("Change log file to %s", szLogFileName.c_str());
	m_Log.Write(szLog);

	string oldLogFile = m_Log.GetFileName();
	m_Log.SetFileName(szLogFileName);
	szLog.Format("New log file change from %s", oldLogFile);
	m_Log.Write(szLog);
}

/// <summary>
/// enable log, write the log or false
/// </summary>
/// <param name="enable"> enable boolean </param>
void SerialPort::EnableLog(bool enable)
{
	if (enable)
		m_Log.Write("Enable log");
	else
		m_Log.Write("Disable log");
	m_Log.Enable(enable);
}

/// <summary>
/// write data to port
/// </summary>
/// <param name="szWriteData">write data</param>
/// <param name="dwWrittenLength">out, the bytes written in the port</param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::Write(const CString& szWriteData, DWORD *dwWrittenLength)
{
	PurgeComm(m_hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);		//clear the input/output buffer

	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; Port isnot open when write data to serial port", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}

	m_Log.Write("Writting data to serial port: " + szWriteData);

	// clear error before read/write
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	ClearCommError(m_hPort, &dwErrorFlags, &ComStat);	

	DWORD bytesWritten(0);	//written size

	//Synchronous(non-overlapped) communication method
	if (m_bSync)
	{
		//write		
		BOOL bWriteStatus = WriteFile(m_hPort, szWriteData, static_cast<DWORD>(szWriteData.GetLength()), &bytesWritten, nullptr);
		if (nullptr != dwWrittenLength)
		{
			*dwWrittenLength = bytesWritten;
		}

		if (!bWriteStatus)
		{
			m_szLogStr.Format("Error %d; error in write data to serial port", ERROR_WRITE_PORT_FAILED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_WRITE_PORT_FAILED;
		}
		//error if written size not equal writedata length
		if (bytesWritten != szWriteData.GetLength())
		{
			m_szLogStr.Format("Error %d; write date not finished because of time out, data size = %d, written size = %d",
				ERROR_WRITE_NOT_FINISH, szWriteData.GetLength(), bytesWritten);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_WRITE_NOT_FINISH;
		}
		//sleep to wait the write function completed, for synchronous communication, + 10 to make sure the time is enough
		double sleepTime = static_cast<double>(szWriteData.GetLength()) * 1000.0 * 8.0 / static_cast<double>(m_dwBaudRate) + 10;
		Sleep(static_cast<DWORD>(sleepTime));

		m_Log.Write("Write data to serial port successfully: " + szWriteData);
		return SUCCESS;
	}
	//Asynchronous(overlapped) communication method
	else
	{
		//create overlapped structure and envent
		::OVERLAPPED osWrite;
		memset(&osWrite, 0, sizeof(osWrite));
		osWrite.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		//write		
		BOOL bWriteStatus = WriteFile(m_hPort, szWriteData, static_cast<DWORD>(szWriteData.GetLength()), &bytesWritten, &osWrite);
		if (!bWriteStatus)
		{
			//write error
			if (ERROR_IO_PENDING != ::GetLastError())
			{
				if (nullptr != dwWrittenLength)
				{
					*dwWrittenLength = bytesWritten;
				}
				::CloseHandle(osWrite.hEvent);	//close handle before return
				m_szLogStr.Format("Error %d; error in write data to serial port", ERROR_WRITE_PORT_FAILED);
				m_Log.Write(m_szLogStr, MyLog::Error);
				return ERROR_WRITE_PORT_FAILED;
			}
			//write is pending
			else
			{
				switch (::WaitForSingleObject(osWrite.hEvent, INFINITE))
				{
				case WAIT_OBJECT_0:
					//overlapped event has been signaled
					//get the overlapped result to check whether the write is finish SUCCESSFULLY
					if(!::GetOverlappedResult(m_hPort, &osWrite, &bytesWritten, FALSE))
					{
						if (nullptr != dwWrittenLength)
						{
							*dwWrittenLength = bytesWritten;
						}
						::CloseHandle(osWrite.hEvent);	//close handle before return
						m_szLogStr.Format("Error %d; error in write data to serial port", ERROR_WRITE_PORT_FAILED);
						m_Log.Write(m_szLogStr, MyLog::Error);
						return ERROR_WRITE_PORT_FAILED;
					}
					else
					{
						if (nullptr != dwWrittenLength)
						{
							*dwWrittenLength = bytesWritten;
						}
						//check the WriteData size is equal the written size
						if (szWriteData.GetLength() == bytesWritten)
						{
							::CloseHandle(osWrite.hEvent);	//close handle before return
							m_Log.Write("Write data to serial port successfully: " + szWriteData);
							return SUCCESS;
						}
						else
						{
							::CloseHandle(osWrite.hEvent);	//close handle before return
							m_szLogStr.Format("Error %d; write date not finished successfully, data size = %d, written size = %d",
								ERROR_WRITE_NOT_FINISH, szWriteData.GetLength(), bytesWritten);
							m_Log.Write(m_szLogStr, MyLog::Error);
							return ERROR_WRITE_NOT_FINISH;
						}
					}
					break;
				case WAIT_TIMEOUT:
					if (nullptr != dwWrittenLength)
					{
						*dwWrittenLength = bytesWritten;
					}
					::CloseHandle(osWrite.hEvent);	//close handle before return
					m_szLogStr.Format("Error %d; write date not finished because of time out, data size = %d, written size = %d",
						ERROR_WRITE_NOT_FINISH, szWriteData.GetLength(), bytesWritten);
					m_Log.Write(m_szLogStr, MyLog::Error);
					return ERROR_WRITE_NOT_FINISH;
				default:
					if (nullptr != dwWrittenLength)
					{
						*dwWrittenLength = bytesWritten;
					}
					::CloseHandle(osWrite.hEvent);	//close handle before return
					m_szLogStr.Format("Error %d; error in write data to serial port", ERROR_WRITE_PORT_FAILED);
					m_Log.Write(m_szLogStr, MyLog::Error);
					return ERROR_WRITE_PORT_FAILED;
				}
			}
		}
		//write data complete immediately, just check the written size
		else
		{
			if (nullptr != dwWrittenLength)
			{
				*dwWrittenLength = bytesWritten;
			}
			::CloseHandle(osWrite.hEvent);	//close handle before return
			//error if written size not equal writedata length
			if (bytesWritten != szWriteData.GetLength())
			{				
				m_szLogStr.Format("Error %d; write date not finished, data size = %d, written size = %d",
					ERROR_WRITE_NOT_FINISH, szWriteData.GetLength(), bytesWritten);
				m_Log.Write(m_szLogStr, MyLog::Error);
				return ERROR_WRITE_NOT_FINISH;
			}
			else
			{
				m_Log.Write("Write data to serial port successfully:" + szWriteData);
				return SUCCESS;
			}
		}		
	}
}

/// <summary>
/// read data form port
/// </summary>
/// <param name="szReadData"> read data </param>
/// <param name="dwMaxReadLength"> the max data length to read </param>
/// <param name="dwReadLength"> out, the actual read Bytes from port </param>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::Read(char* szReadData, DWORD dwMaxReadLength, DWORD *dwReadLength)
{
	//detect whether the port is open
	if (!IsOpen())
	{
		m_szLogStr.Format("Error %d; Port isnot open when read data from serial port", ERROR_PORT_NOT_OPEN);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_PORT_NOT_OPEN;
	}
	m_Log.Write("Reading data from serial port");

	// clear error before read/write
	DWORD dwErrorFlags;
	COMSTAT ComStat;
	ClearCommError(m_hPort, &dwErrorFlags, &ComStat);

	//read
	DWORD dwBytesRead(0);			//record the readed byte size

	//Synchronous(non-overlapped) communication method
	if(m_bSync)
	{
		//-1 to make the szReadData end with '\0', or error occures when print szReadData
		BOOL bReadStatus = ReadFile(m_hPort, szReadData, dwMaxReadLength - 1, &dwBytesRead, nullptr);
		if (nullptr != dwReadLength)
		{
			*dwReadLength = dwBytesRead;
		}

		if (!bReadStatus)
		{
			m_szLogStr.Format("Error %d; Failed to read data from serial port", ERROR_READ_PORT_FAILED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_READ_PORT_FAILED;
		}
		//warning if the szReadData length less than the read data in port quene
		if (ComStat.cbInQue > dwMaxReadLength - 1)
		{
			m_szLogStr.Format("Error %d; Input size isnot enough when read data from serial port", WARNING_READ_DATA_EXCEED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return WARNING_READ_DATA_EXCEED;
		}
		else
		{
			m_szLogStr.Format("Read data from serial port successfully: %s", szReadData);
			m_Log.Write(m_szLogStr);
			return SUCCESS;
		}
	}
	//Asynchronous(overlapped) communication method
	else
	{
		//create overlapped structure and envent, must close before return
		::OVERLAPPED osRead;
		memset(&osRead, 0, sizeof(osRead));		
		osRead.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		//read		
		BOOL bReadStatus = ReadFile(m_hPort, szReadData, dwMaxReadLength - 1, &dwBytesRead, &osRead);
		if (!bReadStatus)
		{
			//read error
			if (ERROR_IO_PENDING != ::GetLastError())
			{
				if (nullptr != dwReadLength)
				{
					*dwReadLength = dwBytesRead;
				}
				::CloseHandle(osRead.hEvent);	//close handle before return
				m_szLogStr.Format("Error %d; Failed to read data from serial port", ERROR_READ_PORT_FAILED);
				m_Log.Write(m_szLogStr, MyLog::Error);
				return ERROR_READ_PORT_FAILED;
			}
			//read is pending
			else
			{
				switch (::WaitForSingleObject(osRead.hEvent, INFINITE))
				{
				case WAIT_OBJECT_0:
					//overlapped event has been signaled
					//get the overlapped result to check whether the read is finish SUCCESSFULLY
					if(!::GetOverlappedResult(m_hPort, &osRead, &dwBytesRead, FALSE))
					{
						if (nullptr != dwReadLength)
						{
							*dwReadLength = dwBytesRead;
						}
						::CloseHandle(osRead.hEvent);	//close handle before return
						m_szLogStr.Format("Error %d; Failed to read data from serial port", ERROR_READ_PORT_FAILED);
						m_Log.Write(m_szLogStr, MyLog::Error);
						return ERROR_READ_PORT_FAILED;
					}
					else
					{
						if (nullptr != dwReadLength)
						{
							*dwReadLength = dwBytesRead;
						}
						//warning if the szReadData length less than the read data in port quene
						if (ComStat.cbInQue > dwMaxReadLength - 1)
						{
							::CloseHandle(osRead.hEvent);	//close handle before return
							m_szLogStr.Format("Error %d; Input size isnot enough when read data from serial port", WARNING_READ_DATA_EXCEED);
							m_Log.Write(m_szLogStr, MyLog::Error);
							return WARNING_READ_DATA_EXCEED;
						}
						else
						{
							::CloseHandle(osRead.hEvent);	//close handle before return
							m_szLogStr.Format("Read data from serial port successfully: %s", szReadData);
							m_Log.Write(m_szLogStr);
							return SUCCESS;
						}
					}
					break;
				case WAIT_TIMEOUT:
					if (nullptr != dwReadLength)
					{
						*dwReadLength = dwBytesRead;
					}
					::CloseHandle(osRead.hEvent);	//close handle before return
					m_szLogStr.Format("Error %d; Time out when read data from serial port", ERROR_READ_PORT_TIMEOUT);
					m_Log.Write(m_szLogStr, MyLog::Error);
					return ERROR_READ_PORT_TIMEOUT;
				default:
					if (nullptr != dwReadLength)
					{
						*dwReadLength = dwBytesRead;
					}
					::CloseHandle(osRead.hEvent);	//close handle before return
					m_szLogStr.Format("Error %d; Failed to read data from serial port", ERROR_READ_PORT_FAILED);
					m_Log.Write(m_szLogStr, MyLog::Error);
					return ERROR_READ_PORT_FAILED;
				}
			}
		}
		//read data complete immediately, just check the szReadData size
		else
		{
			if (nullptr != dwReadLength)
			{
				*dwReadLength = dwBytesRead;
			}
			::CloseHandle(osRead.hEvent);	//close handle before return
			//warning if the szReadData length less than the read data in port quene
			if (ComStat.cbInQue > dwMaxReadLength - 1)
			{
				m_szLogStr.Format("Error %d; Input size isnot enough when read data from serial port, ", WARNING_READ_DATA_EXCEED);
				m_Log.Write(m_szLogStr, MyLog::Error);
				return WARNING_READ_DATA_EXCEED;
			}
			else
			{
				m_szLogStr.Format("Read data from serial port successfully: %s", szReadData);
				m_Log.Write(m_szLogStr);
				return SUCCESS;
			}
		}
	}
}

/// <summary>
/// set the comm state of serial port
/// </summary>
/// <returns>
/// return SUCCESS if set OK, otherwise return ERROR Code
/// </returns>
int SerialPort::set()
{
	//get comm state
	DCB dcb;
	BOOL bStatus = GetCommState(m_hPort, &dcb);
	if (!bStatus)
	{
		m_szLogStr.Format("Error %d; Fail to get communication state", ERROR_GET_COMM_STATE_FALIED);
		m_Log.Write(m_szLogStr, MyLog::Error);
		return ERROR_GET_COMM_STATE_FALIED;
	}

	// set comm state
	dcb.BaudRate = m_dwBaudRate;
	dcb.ByteSize = m_nByteSize;
	dcb.Parity = m_nParity;
	dcb.StopBits = m_nStopBits;
	bStatus = SetCommState(m_hPort, &dcb);
	if (!bStatus)
	{
		switch (GetLastError())
		{
		case ERROR_INVALID_PARAMETER:
			m_szLogStr.Format("Error %d; Invalid communication state parameters in set communication state", ERROR_INVALID_SET_PARAMETER);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_INVALID_SET_PARAMETER;
			break;
		default:
			m_szLogStr.Format("Error %d; Failed to set communication state", ERROR_PORT_SETTING_FAILED);
			m_Log.Write(m_szLogStr, MyLog::Error);
			return ERROR_PORT_SETTING_FAILED;
			break;
		}		
	}

	m_szLogStr.Format("Set baud rate = %d, B,S,P = %d,%d,%d", m_dwBaudRate, m_nByteSize, m_nStopBits, m_nParity);
	m_Log.Write(m_szLogStr);
	return SUCCESS;
}
