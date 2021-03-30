#include "stdafx.h"
#include "TeleinfoTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

CTeleinfoTCP::CTeleinfoTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, int datatimeout, const bool disable_crc, const int ratelimit):
	m_szIPAddress(IPAddress),
	m_usIPPort(usIPPort)
{
	m_HwdID = ID;
	m_iDataTimeout = datatimeout;
	m_bDisableCRC = disable_crc;
	m_iRateLimit = ratelimit;
	
	m_iBaudRate = 1200;

	if ((m_iRateLimit > m_iDataTimeout) && (m_iDataTimeout > 0))  m_iRateLimit = m_iDataTimeout;
	Init();
}

void CTeleinfoTCP::Init()
{
	InitTeleinfo();
}

bool CTeleinfoTCP::StartHardware()
{
	RequestStart();

	m_bIsStarted = true;

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}


bool CTeleinfoTCP::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}


void CTeleinfoTCP::Do_Work()
{
	_log.Log(LOG_STATUS, "(%s) attempt connect to %s:%d",  m_Name.c_str(), m_szIPAddress.c_str(), m_usIPPort);
	connect(m_szIPAddress, m_usIPPort);
	while (!IsStopRequested(1000))
	{
	}
	terminate();

	_log.Log(LOG_STATUS, "(%s): TCP/IP Worker stopped...",  m_Name.c_str());
}


bool CTeleinfoTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	return isConnected();
}


void CTeleinfoTCP::OnConnect()
{
	Init();

	_log.Log(LOG_STATUS, "(%s) connected to: %s:%d",  m_Name.c_str(), m_szIPAddress.c_str(), m_usIPPort);

	if (m_bDisableCRC)
		_log.Log(LOG_STATUS, "(%s) CRC checks on incoming data are disabled", m_Name.c_str());
	else
		_log.Log(LOG_STATUS, "(%s) CRC checks will be performed on incoming data", m_Name.c_str());
}


void CTeleinfoTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS, "(%s) disconnected",  m_Name.c_str());
}


void CTeleinfoTCP::OnData(const unsigned char *pData, size_t length)
{
	ParseTeleinfoData((const char *)pData, static_cast<int>(length));
}


void CTeleinfoTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "(%s) Can not connect to: %s:%d",  m_Name.c_str(), m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "(%s) Connection reset!",  m_Name.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR, "(%s) %s",  m_Name.c_str(), error.message().c_str());
	}
}
