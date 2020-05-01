/*
  _________.__    .___      __   .__        __        _________   ________   _____  
 /   _____/|__| __| _/____ |  | _|__| ____ |  | __    \_   ___ \ /  _____/  /  |  | 
 \_____  \ |  |/ __ |/ __ \|  |/ /  |/ ___\|  |/ /    /    \  \//   __  \  /   |  |_
 /        \|  / /_/ \  ___/|    <|  \  \___|    <     \     \___\  |__\  \/    ^   /
/_______  /|__\____ |\___  >__|_ \__|\___  >__|_ \     \______  /\_____  /\____   | 
        \/         \/    \/     \/       \/     \/            \/       \/      |__| 
 
 net.cpp

 RasPiC64 - A framework for interfacing the C64 and a Raspberry Pi 3B/3B+
          - misc code
 Copyright (c) 2019, 2020 Carsten Dachsbacher <frenetic@dachsbacher.de>

 Network related code in this file contributed by Henning Pingel based on 
 the networking examples within Rene Stanges Circle framework.

 Logo created with http://patorjk.com/software/taag/
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "net.h"
#include "helpers.h"

//#include <circle/net/ntpdaemon.h>
#include <circle/net/ntpclient.h>
#include <circle/net/dnsclient.h>
#include <circle/net/httpclient.h>
// Network configuration
#define USE_DHCP
// Time configuration
static const char NTPServer[]    = "pool.ntp.org";
static const int nTimeZone       = 2*60;		// minutes diff to UTC
static const char DRIVE[] = "SD:";
static const unsigned nDocMaxSize = 800*1024;
// Sidekick Developer HTTP Server configuration
//Fill this value with the hostname of your development webserver
//to activate the automated kernel file update via HTTP 
static const char KernelUpdateFile[] = "/kernel8.img";
//change the path of KernelUpdateFile to your needs
//nDocMaxSize reserved 800 KB as the maximum size of the kernel file
static const char FILENAME_HTTPDUMP[] = "SD:kernel8.img";

CSidekickNet::CSidekickNet( CInterruptSystem * pInterruptSystem, CTimer * pTimer, CScheduler * pScheduler  )
		:m_USBHCI ( pInterruptSystem, pTimer),
		m_pScheduler( pScheduler ),
		m_pTimer ( pTimer ),
		m_isActive( false ),
		m_FileLength( 0 )
{
	assert (m_pTimer != 0);
	assert (& m_pScheduler != 0);
	assert (& m_USBHCI != 0);

	#ifdef NET_DEV_SERVER
	  m_DevHttpHost = (const char *) NET_DEV_SERVER;
		m_SidekickKernelLocation = FILENAME_HTTPDUMP;
	#else
	  m_DevHttpHost = 0;
		m_SidekickKernelLocation = 0;
	#endif
	
	//timezone is not really related to net stuff, it could go somewhere else
	m_pTimer->SetTimeZone (nTimeZone);
}

boolean CSidekickNet::Initialize()
{
	if (m_isActive)
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Strange: Network is already up and running. Skipping another init."
		);
		return true;
	}
	if (!m_USBHCI.Initialize ())
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Couldn't initialize instance of CUSBHCIDevice."
		);
		return false;
	}
	
	if (!m_Net.Initialize (false))
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Couldn't initialize instance of CNetSubSystem."
		);
		return false;
	}
	
	unsigned sleepCount = 0;
	static const unsigned sleepLimit = 100;
	while (!m_Net.IsRunning () && sleepCount < sleepLimit)
	{
		m_pScheduler->MsSleep (100);
		sleepCount ++;
	}
	
	if (!m_Net.IsRunning () && sleepCount >= sleepLimit){
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Network connection is not running - is ethernet cable not attached?"
		);
		return false;
	}

	//net connection is up and running now
	m_isActive = true;

	//TODO: Decide if we really need an ntp daemon or if we can get away with
	//a simpler single ntpclient call, that requires more lines of code
	//new CNTPDaemon ("pool.ntp.org", &m_Net);
	return true;
}

boolean CSidekickNet::UpdateTime(void)
{
	assert (m_isActive);
	CIPAddress NTPServerIP;
	CDNSClient DNSClient (&m_Net);
	if (!DNSClient.Resolve (NTPServer, &NTPServerIP))
	{
		CLogger::Get ()->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot resolve: %s",
					(const char *) NTPServer);

		return false;
	}
	
	CNTPClient NTPClient (&m_Net);
	unsigned nTime = NTPClient.GetTime (NTPServerIP);
	if (nTime == 0)
	{
		CLogger::Get ()->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot get time from %s",
					(const char *) NTPServer);

		return false;
	}

	if (CTimer::Get ()->SetTime (nTime, FALSE))
	{
		CLogger::Get ()->Write ("CSidekickNet::UpdateTime", LogNotice, "System time updated");
	}
	else
	{
		CLogger::Get ()->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot update system time");
	}

	return false;
}


//example that looks for the presence of a file on a pre-defined HTTP Server
//file is being read and preserved in memory
//for the purpose to replace the kernel8.img file on SD card
//in method StoreSidekickKernelFile
boolean CSidekickNet::CheckForSidekickKernelUpdate()
{
	if ( m_DevHttpHost == 0 )
	{
		logger->Write( "CSidekickNet::CheckForFirmwareUpdate", LogNotice, 
			"Skipping check: NET_DEV_SERVER is not defined."
		);
		return FALSE;
	}
	assert (m_isActive);
		
	m_FileLength = 0;
	m_pFileBuffer = new char[nDocMaxSize+1];	// +1 for 0-termination
	if (m_pFileBuffer == 0)
	{
		logger->Write( "CSidekickNet::CheckForFirmwareUpdate", LogPanic, "Cannot allocate document buffer");
	}
	logger->Write( "CSidekickNet::CheckForFirmwareUpdate", LogNotice, 
		"Now fetching kernel file from NET_DEV_SERVER."
	);
	m_storeFile = GetFileViaHTTP ( m_DevHttpHost, KernelUpdateFile, m_pFileBuffer, m_FileLength);
	return m_storeFile;
}

//the only reason why this method is split from the above one is for the
//sake of shady experiments with improving the boot duration
boolean CSidekickNet::StoreSidekickKernelFile()
{
	if ( m_storeFile ){
		logger->Write( "SidekickKernelUpdater", LogNotice, 
			"Now trying to write kernel file to SD card, bytes to write: %i", m_FileLength
		);
		writeFile( logger, DRIVE, FILENAME_HTTPDUMP, (u8*) m_pFileBuffer, m_FileLength );
		logger->Write( "SidekickKernelUpdater", LogNotice, "Finished writing kernel to SD card");
	}
	m_pFileBuffer = new char[1];//make it very small
	m_pFileBuffer = 0;	
	m_pScheduler->Sleep (2); // TODO: this is ugly
	m_storeFile = false;
	m_FileLength = 0;
	return true;
}

CNetConfig * CSidekickNet::GetNetConfig(){
	return m_Net.GetConfig ();
}

//temporary test method to do a HTTP request just to get a "small" 404 reply
void CSidekickNet::contactDevServer(){
	assert (m_isActive);
	m_FileLength = 0;
	m_pFileBuffer = new char[100];	// +1 for 0-termination
	if (m_pFileBuffer == 0)
	{
		logger->Write( "contactDevServer", LogPanic, "Cannot allocate document buffer");
	}
	//logger->Write( "contactDevServer", LogNotice, "Now fetching 404 file from NET_DEV_SERVER.");
	const char * file404 = "not_there.html";
	boolean bTmp = GetFileViaHTTP ( m_DevHttpHost, file404, m_pFileBuffer, m_FileLength);
	m_pFileBuffer = new char[1];
	m_pFileBuffer = 0;	
}

boolean CSidekickNet::GetFileViaHTTP (const char * pHost, const char * pFile, char *pBuffer, unsigned & nLengthRead)
{
	//This method is derived from the webclient example of circle.
	assert (m_isActive);
	CIPAddress ForeignIP;
	CDNSClient DNSClient (&m_Net);
	if (!DNSClient.Resolve (pHost, &ForeignIP))
	{
		logger->Write( "GetFileViaHTTP", LogError, "Cannot resolve: %s", pHost);

		return false;
	}

	CString IPString;
	ForeignIP.Format (&IPString);
	logger->Write( "GetFileViaHTTP", LogDebug, "Outgoing connection to %s", (const char *) IPString);

	unsigned nLength = nDocMaxSize;

	assert (pBuffer != 0);
	CHTTPClient Client (&m_Net, ForeignIP, HTTP_PORT, pHost);
	THTTPStatus Status = Client.Get (pFile, (u8 *) pBuffer, &nLength);
	if (Status != HTTPOK)
	{
		logger->Write( "GetFileViaHTTP", LogError, "HTTP request failed (status %u)", Status);

		return false;
	}

	logger->Write( "GetFileViaHTTP", LogDebug, "%u bytes successfully received", nLength);

	assert (nLength <= nDocMaxSize);
	pBuffer[nLength] = '\0';
	nLengthRead = nLength;

	return TRUE;
}
