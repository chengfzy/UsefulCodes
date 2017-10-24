#include "SerialPort.h"
#include <iostream>
#include <afx.h>
#include <cmath>

using namespace std;
using namespace io;

int main()
{
	SerialPort portA("COM1");
	portA.Open(false);
	SerialPort portB("COM2");
	portB.Open(false);

	char readData[512];	
	CString writeData = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ###000###0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	

	for (int i = 0; i < 100; ++i)
	{
		memset(readData, 0, 512);
		portA.Write(writeData);
		
		portB.Read(readData, 512);
		CString readStr(readData);
		
		cout << "[" << i << "] " << readStr << endl;
	}

	portA.Close();
	portB.Close();

	system("pause");
	return 0;
}