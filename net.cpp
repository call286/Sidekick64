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

#include <circle/net/ntpclient.h>
#include <circle/net/httpclient.h>
// Network configuration
#ifndef WITH_WLAN
#define USE_DHCP
#endif
// Time configuration
static const char NTPServer[]    = "pool.ntp.org";
static const int nTimeZone       = 2*60;		// minutes diff to UTC
static const char DRIVE[] = "SD:";
//nDocMaxSize reserved 800 KB as the maximum size of the kernel file
static const unsigned nDocMaxSize = 900*1024;
static const char KERNEL_IMG_NAME[] = "kernel8.img";
static const char KERNEL_SAVE_LOCATION[] = "SD:kernel8.img";
static const char RPIMENU64_SAVE_LOCATION[] = "SD:C64/rpimenu.prg";
static const char msgNoConnection[] = "Sorry, no network connection!";
static const char msgNotFound[]     = "Message not found. :(";
static const char NMOTDfile[]       = "/not_there.php";
static const char * kernelUpdatePath[2] = { "/sidekick64/kernel8.img", "/sidekick264/kernel8.img"};
static const char * prgUpdatePath[2] = { "/sidekick64/rpimenu.prg", "/sidekick264/rpimenu.prg"};

#ifdef WITH_WLAN
#define DRIVE		"SD:"
#define FIRMWARE_PATH	DRIVE "/firmware/"		// firmware files must be provided here
#define CONFIG_FILE	DRIVE "/wpa_supplicant.conf"
#endif

CSidekickNet::CSidekickNet( CInterruptSystem * pInterruptSystem, CTimer * pTimer, CScheduler * pScheduler, CEMMCDevice * pEmmcDevice  )
		:m_USBHCI (pInterruptSystem, pTimer),
		m_pScheduler(pScheduler),
		m_pTimer (pTimer),
#ifdef WITH_WLAN		
		m_EMMC ( *pEmmcDevice),
		m_WLAN (FIRMWARE_PATH),
		m_Net (0, 0, 0, 0, DEFAULT_HOSTNAME, NetDeviceTypeWLAN),
		m_WPASupplicant (CONFIG_FILE),
#endif
		m_DNSClient(&m_Net),
		m_useWLAN (false),
		m_isActive( false ),
		m_isPrepared( false ),
		m_isNetworkInitQueued( false ),
		m_isKernelUpdateQueued( false ),
		m_isNMOTDQueued( false ),
		m_devServerMessage( (char *) "Press M to see another message here." ),
		m_networkActionStatusMsg( (char * ) ""),
		m_sktxScreenContent( (char * ) ""),
		m_PiModel( m_pMachineInfo->Get()->GetMachineModel () ),
		m_DevHttpHost(0),
		m_PlaygroundHttpHost(0),
		m_SidekickKernelUpdatePath(0),
		m_queueDelay(0),
		m_effortsSinceLastEvent(0),
		m_sktxKey(0)
{
	assert (m_pTimer != 0);
	assert (& m_pScheduler != 0);
	assert (& m_USBHCI != 0);

	#ifdef NET_DEV_SERVER
	  m_DevHttpHost = (const char *) NET_DEV_SERVER;
	#else
	  m_DevHttpHost = 0;
	#endif
	
	#ifdef NET_PUBLIC_PLAY_SERVER
	  m_PlaygroundHttpHost = (const char *) NET_PUBLIC_PLAY_SERVER;
	#else
	  m_PlaygroundHttpHost = 0;
	#endif

	#ifdef WITH_WLAN
		m_useWLAN = true;
	#else
		m_useWLAN = false;
	#endif

	//timezone is not really related to net stuff, it could go somewhere else
	m_pTimer->SetTimeZone (nTimeZone);
}

void CSidekickNet::setSidekickKernelUpdatePath( unsigned type)
{
	m_SidekickKernelUpdatePath = type;
};

boolean CSidekickNet::Initialize()
{
	const unsigned sleepLimit = 100 * (m_useWLAN ? 10:1);
	unsigned sleepCount = 0;
	
	if (m_isActive)
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Strange: Network is already up and running. Skipping another init."
		);
		return true;
	}
	if ( !m_isPrepared )
	{
		if (!Prepare()){
			return false;
		}
		m_isPrepared = true;
	}
	while (!m_Net.IsRunning () && sleepCount < sleepLimit)
	{
		m_pScheduler->MsSleep (100);
		sleepCount ++;
	}
	if ( m_useWLAN )
		if ( !unmountSDDrive() )
			return false;

	if (!m_Net.IsRunning () && sleepCount >= sleepLimit){
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Network connection is not running - is ethernet cable not attached?"
		);
		return false;
	}

	//net connection is up and running now
	m_isActive = true;
	
	//TODO: these resolves could be postponed to the moment where the first 
	//actual access takes place
	if ( m_DevHttpHost != 0)
		m_DevHttpServerIP = getIPForHost(m_DevHttpHost);
	if ( m_PlaygroundHttpHost != 0)
		m_PlaygroundHttpServerIP = getIPForHost( m_PlaygroundHttpHost);
	return true;
}

boolean CSidekickNet::unmountSDDrive()
{
	if (f_mount ( 0, DRIVE, 0) != FR_OK)
	{
		logger->Write ("CSidekickNet::Initialize", LogError,
				"Cannot unmount drive: %s", DRIVE);
		return false;
	}
	return true;
}		

boolean CSidekickNet::RaspiHasOnlyWLAN()
{
	return (m_PiModel == MachineModel3APlus );
}

boolean CSidekickNet::Prepare()
{
	if ( m_PiModel != MachineModel3APlus && m_PiModel != MachineModel3BPlus)
	{
		logger->Write( "CSidekickNet::Initialize", LogWarning, 
			"Warning: The model of Raspberry Pi you are using is not a model supported by Sidekick64/264!"
		);
	}
	
	if ( RaspiHasOnlyWLAN() && !m_useWLAN )
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Your Raspberry Pi model (3A+) doesn't have an ethernet socket. Skipping init of CNetSubSystem."
		);
		return false;
	}
	
	if (!m_USBHCI.Initialize ())
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Couldn't initialize instance of CUSBHCIDevice."
		);
		return false;
	}
	if (m_useWLAN)
	{
		#ifdef WITH_WLAN
		if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
		{
			logger->Write ("CSidekickNet::Initialize", LogError,
					"Cannot mount drive: %s", DRIVE);

			return false;
		}
		if (!m_WLAN.Initialize ())
		{
			logger->Write( "CSidekickNet::Initialize", LogNotice, 
				"Couldn't initialize instance of WLAN."
			);
			return false;
		}
		#endif
	}

	if (!m_Net.Initialize (false))
	{
		logger->Write( "CSidekickNet::Initialize", LogNotice, 
			"Couldn't initialize instance of CNetSubSystem."
		);
		return false;
	}
	if (m_useWLAN)
	{
	#ifdef WITH_WLAN
		if (!m_WPASupplicant.Initialize ())
		{
			logger->Write( "CSidekickNet::Initialize", LogNotice, 
				"Couldn't initialize instance of CWPASupplicant."
			);
			return false;
		}
		#endif
	}
	return true;
}

boolean CSidekickNet::IsRunning ()
{
	 return m_isActive; 
}

void CSidekickNet::queueNetworkInit()
{ 
	m_isNetworkInitQueued = true;
	m_networkActionStatusMsg = (char*) "Trying to connect. Please wait.";
	m_queueDelay = 1;
}

void CSidekickNet::queueKernelUpdate()
{ 
	m_isKernelUpdateQueued = true; 
	m_networkActionStatusMsg = (char*) "Trying to update kernel. Please wait.";
	m_queueDelay = 1;
}

void CSidekickNet::queueNetworkMessageOfTheDay()
{
 	m_isNMOTDQueued = true;
	m_networkActionStatusMsg = (char*) "Trying to get NMOTD. Please wait.";
	m_queueDelay = 1;
}

void CSidekickNet::queueSktxKeypress( int key )
{
	m_isSktxKeypressQueued = true;
	m_sktxKey = key;
	m_queueDelay = 0;
}

void CSidekickNet::queueSktxRefresh()
{
	m_skipSktxRefresh++;
	if ( m_skipSktxRefresh > 50 )
	{
		m_skipSktxRefresh = 0;
		queueSktxKeypress( 92 );
	}
}


void CSidekickNet::handleQueuedNetworkAction()
{
	#ifdef WITH_WLAN
	if (m_isActive && !isAnyNetworkActionQueued())
	{
		m_effortsSinceLastEvent++;
		if ( m_effortsSinceLastEvent > 200)
		{
			//as a WLAN keep-alive, auto queue a network event / HTTP request
			//to avoid WLAN going into "zombie" disconnected mode
			m_isNMOTDQueued = true;
			m_effortsSinceLastEvent = 0;
		}
	}
	#endif
	
	if (m_queueDelay > 0 )
	{
		m_queueDelay--;
		return;
	}

	/*
	if ( 	m_networkActionStatusMsg != (char *) "")
	{
		m_networkActionStatusMsg = (char *) "";
		return;
	}
*/
	if ( m_isNetworkInitQueued && !m_isActive )
	{
		assert (!m_isActive);
		if (Initialize())
		{
			unsigned tries = 0;
			while (!UpdateTime() && tries < 3){ tries++;};
		}
		m_isNetworkInitQueued = false;
		m_effortsSinceLastEvent = 0;
		return;
	}
	else if (m_isActive)
	{
		if ( m_isKernelUpdateQueued )
		{
		 	CheckForSidekickKernelUpdate();
			m_isKernelUpdateQueued = false;
			m_effortsSinceLastEvent = 0;
 		}
		else if (m_isNMOTDQueued)
		{
			updateNetworkMessageOfTheDay();
			m_isNMOTDQueued = false;
			m_effortsSinceLastEvent = 0;
		}
		else if (m_isSktxKeypressQueued)
		{
			updateSktxScreenContent();
			m_isSktxKeypressQueued = false;
			m_effortsSinceLastEvent = 0;
		}
	}
}

boolean CSidekickNet::isAnyNetworkActionQueued()
{
	return m_isNetworkInitQueued || m_isKernelUpdateQueued || m_isNMOTDQueued || m_isSktxKeypressQueued;
}

char * CSidekickNet::getNetworkActionStatusMessage()
{
	return m_networkActionStatusMsg;
}


CString CSidekickNet::getTimeString()
{
	//the most complicated and ugly way to get the data into the right form...
	CString *pTimeString = m_pTimer->GetTimeString();
	CString Buffer;
	if (pTimeString != 0)
	{
		Buffer.Append (*pTimeString);
	}
	delete pTimeString;
	//logger->Write( "getTimeString", LogDebug, "%s ", Buffer);
	return Buffer;
}

CString CSidekickNet::getRaspiModelName()
{
	return m_pMachineInfo->Get()->GetMachineName ();
}

CNetConfig * CSidekickNet::GetNetConfig(){
	assert (m_isActive);
	return m_Net.GetConfig ();
}

char * CSidekickNet::getNetworkMessageOfTheDay(){
	return m_devServerMessage;
}

CIPAddress CSidekickNet::getIPForHost( const char * host )
{
	assert (m_isActive);
	CIPAddress ip;
	if (!m_DNSClient.Resolve (host, &ip))
		logger->Write ("CSidekickNet::getIPForHost", LogWarning, "Cannot resolve: %s",host);
	else
		logger->Write ("CSidekickNet::getIPForHost", LogNotice, "DNS resolve ok for: %s",host);
	return ip;
}

boolean CSidekickNet::UpdateTime(void)
{
	assert (m_isActive);
	CIPAddress NTPServerIP;
	if (!m_DNSClient.Resolve (NTPServer, &NTPServerIP))
	{
		logger->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot resolve: %s",
					(const char *) NTPServer);

		return false;
	}
	CNTPClient NTPClient (&m_Net);
	unsigned nTime = NTPClient.GetTime (NTPServerIP);
	if (nTime == 0)
	{
		logger->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot get time from %s",
					(const char *) NTPServer);

		return false;
	}

	if (CTimer::Get ()->SetTime (nTime, FALSE))
	{
		logger->Write ("CSidekickNet::UpdateTime", LogNotice, "System time updated");
		return true;
	}
	else
	{
		logger->Write ("CSidekickNet::UpdateTime", LogWarning, "Cannot update system time");
	}
	return false;
}

//looks for the presence of a file on a pre-defined HTTP Server
//file is being read and stored on the sd card
boolean CSidekickNet::CheckForSidekickKernelUpdate()
{
	if ( m_DevHttpHost == 0 )
	{
		logger->Write( "CSidekickNet::CheckForSidekickKernelUpdate", LogNotice, 
			"Skipping check: NET_DEV_SERVER is not defined."
		);
		return false;
	}
	/*
	if ( m_SidekickKernelUpdatePath == 0 )
	{
		logger->Write( "CSidekickNet::CheckForSidekickKernelUpdate", LogNotice, 
			"Skipping check: HTTP update path is not defined."
		);
		return false;
	}*/
	assert (m_isActive);
	unsigned iFileLength = 0;
	char * pFileBuffer = new char[nDocMaxSize+1];	// +1 for 0-termination
	if (pFileBuffer == 0)
	{
		logger->Write( "CSidekickNet::CheckForFirmwareUpdate", LogError, "Cannot allocate document buffer");
		return false;
	}

	unsigned type = 0;
	if ( m_SidekickKernelUpdatePath == 264 ) type = 1;
		 
	if ( GetHTTPResponseBody ( m_DevHttpServerIP, m_DevHttpHost, kernelUpdatePath[type], pFileBuffer, iFileLength))
	{
		logger->Write( "SidekickKernelUpdater", LogNotice, 
			"Now trying to write kernel file to SD card, bytes to write: %i", iFileLength
		);
		writeFile( logger, DRIVE, KERNEL_SAVE_LOCATION, (u8*) pFileBuffer, iFileLength );
		m_pScheduler->MsSleep (500);
		logger->Write( "SidekickKernelUpdater", LogNotice, "Finished writing kernel to SD card");
	}
	if ( GetHTTPResponseBody ( m_DevHttpServerIP, m_DevHttpHost, prgUpdatePath[type], pFileBuffer, iFileLength))
	{
		logger->Write( "SidekickKernelUpdater", LogNotice, 
			"Now trying to write rpimenu.prg file to SD card, bytes to write: %i", iFileLength
		);
		writeFile( logger, DRIVE, RPIMENU64_SAVE_LOCATION, (u8*) pFileBuffer, iFileLength );
		m_pScheduler->MsSleep (500);
		logger->Write( "SidekickKernelUpdater", LogNotice, "Finished writing file rpimenu.prg to SD card");
	}
	
	return true;
}

void CSidekickNet::updateNetworkMessageOfTheDay(){
	if (!m_isActive || m_PlaygroundHttpHost == 0)
	{
		m_devServerMessage = (char *) msgNoConnection; //FIXME: there's a memory leak in here
		return;
	}
	unsigned iFileLength = 0;
	char * pResponseBuffer = new char[301];	// +1 for 0-termination
	if (pResponseBuffer == 0)
	{
		logger->Write( "updateNetworkMessageOfTheDay", LogError, "Cannot allocate document buffer");
		return;
	}
	if (GetHTTPResponseBody ( m_PlaygroundHttpServerIP, m_PlaygroundHttpHost, NMOTDfile, pResponseBuffer, iFileLength))
	{
		if ( iFileLength > 0 )
			pResponseBuffer[iFileLength-1] = '\0';
		m_devServerMessage = pResponseBuffer;
		pResponseBuffer = 0;
		//logger->Write( "updateNetworkMessageOfTheDay", LogNotice, "HTTP Document content: '%s'", pResponseBuffer);
	}
	else
	{
		m_devServerMessage = (char *) msgNotFound;
	}
}

void CSidekickNet::updateSktxScreenContent(){
	if (!m_isActive || m_PlaygroundHttpHost == 0)
	{
		m_sktxScreenContent = (char *) msgNoConnection; //FIXME: there's a memory leak in here
		return;
	}
	unsigned iFileLength = 0;
	char * pResponseBuffer = new char[2048];	// +1 for 0-termination
	if (pResponseBuffer == 0)
	{
		logger->Write( "updateSktxScreenContent", LogError, "Cannot allocate document buffer");
		return;
	}
  CString path = "/sktx.php?key=";
	CString Number; 
	Number.Format ("%02X", m_sktxKey);
	path.Append( Number );
	m_sktxKey = 0;
	if (GetHTTPResponseBody ( m_PlaygroundHttpServerIP, m_PlaygroundHttpHost, path, pResponseBuffer, iFileLength))
	{
		if ( iFileLength > 0 )
			pResponseBuffer[iFileLength-1] = '\0';
		m_sktxScreenContent = pResponseBuffer;
		pResponseBuffer = 0;
		//logger->Write( "updateNetworkMessageOfTheDay", LogNotice, "HTTP Document content: '%s'", pResponseBuffer);
	}
	else
	{
		m_sktxScreenContent = (char *) msgNotFound;
	}
}


boolean CSidekickNet::GetHTTPResponseBody ( CIPAddress ip, const char * pHost, const char * pFile, char *pBuffer, unsigned & nLengthRead)
{
	//This method is derived from the webclient example of circle.
	assert (m_isActive);
	assert (pBuffer != 0);
	CString IPString;
	ip.Format (&IPString);
	logger->Write( "GetHTTPResponseBody", LogNotice, 
		"HTTP GET to http://%s%s (%s)", pHost, pFile, (const char *) IPString);
	unsigned nLength = nDocMaxSize;
	CHTTPClient Client (&m_Net, ip, HTTP_PORT, pHost);
	THTTPStatus Status = Client.Get (pFile, (u8 *) pBuffer, &nLength);
	if (Status != HTTPOK)
	{
		logger->Write( "GetHTTPResponseBody", LogError, "HTTP request failed (status %u)", Status);

		return false;
	}
	//logger->Write( "GetHTTPResponseBody", LogDebug, "%u bytes successfully received", nLength);
	assert (nLength <= nDocMaxSize);
	pBuffer[nLength] = '\0';
	nLengthRead = nLength;
	return true;
}
