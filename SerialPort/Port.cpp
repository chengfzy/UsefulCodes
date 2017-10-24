#include "Port.h"
using namespace io;
using namespace std;

Port::Port(const string& szChannelName)
	: m_hPort(nullptr), m_szChannelName(szChannelName)
{
}


Port::~Port(void)
{
}


/// <summary>
/// whether the port is open or valid
/// </summary>
/// <returns>
/// return true if open, otherwise return false
/// </returns>
bool Port::IsOpen() const
{
	if (nullptr != m_hPort)
		return true;
	else
		return false;
}

/// <summary>
/// Get the port handle, for some more setting if user need
/// </summary>
/// <returns>
/// Port Handle
/// </returns>
HANDLE Port::GetPortHandle()
{
	return m_hPort;
}
