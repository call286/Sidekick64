/*
  _________.__    .___      __   .__        __        _________   ________   _____  
 /   _____/|__| __| _/____ |  | _|__| ____ |  | __    \_   ___ \ /  _____/  /  |  | 
 \_____  \ |  |/ __ |/ __ \|  |/ /  |/ ___\|  |/ /    /    \  \//   __  \  /   |  |_
 /        \|  / /_/ \  ___/|    <|  \  \___|    <     \     \___\  |__\  \/    ^   /
/_______  /|__\____ |\___  >__|_ \__|\___  >__|_ \     \______  /\_____  /\____   | 
        \/         \/    \/     \/       \/     \/            \/       \/      |__| 
 
 net.h

 RasPiC64 - A framework for interfacing the C64 and a Raspberry Pi 3B/3B+
          - misc code and lots of macros
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

#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>
#include <circle/sched/scheduler.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/util.h>
#include <SDCard/emmc.h>

#ifdef WITH_WLAN
#include <fatfs/ff.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>
#endif

#include <circle/net/netsubsystem.h>
#include <circle/net/dnsclient.h>

#ifndef _sidekicknet_h
#define _sidekicknet_h

extern CLogger *logger;

class CSidekickNet
{
public:
	CSidekickNet( CInterruptSystem *, CTimer *, CScheduler *, CEMMCDevice * );
	~CSidekickNet( void )
	{
	};
	CSidekickNet * GetPointer(){ return this; };
	boolean Initialize ( void );
	boolean IsRunning ( void );
	boolean CheckForSidekickKernelUpdate ();
	boolean GetHTTPResponseBody (CIPAddress, const char * pHost, int port, const char * pFile, char *pBuffer, unsigned & nLengthRead);
	boolean UpdateTime (void);
	void updateFrame();
	void updateSktxScreenContent();
	void queueNetworkInit();
	void queueKernelUpdate();
	void queueFrameRequest();
	void queueSktxKeypress( int );
	void queueSktxRefresh();
	void handleQueuedNetworkAction();
	void setSidekickKernelUpdatePath( unsigned type);
	boolean isAnyNetworkActionQueued();
	boolean isDevServerConfigured(){ return m_DevHttpHost != 0;};
	boolean isWireless(){ return m_useWLAN;};
	boolean RaspiHasOnlyWLAN();
	boolean IsSktxScreenContentEndReached();
	boolean IsSktxScreenToBeCleared();
	boolean IsSktxScreenUnchanged();
  char * getNetworkActionStatusMessage();
	unsigned char * getSktxScreenContent(){ return m_sktxScreenContent; };
	unsigned char * GetSktxScreenContentChunk( u16 & startPos, u8 &color );
	CString getTimeString();
	CNetConfig * GetNetConfig();
	CString getRaspiModelName();
	void ResetSktxScreenContentChunks();
	void setErrorMsgC64( char * msg );

private:
	
	boolean Prepare ( void );
	boolean unmountSDDrive();
	CIPAddress getIPForHost( const char * );
	
	
	CUSBHCIDevice     m_USBHCI;
	CMachineInfo      * m_pMachineInfo; //used for c64screen to display raspi model name
	CScheduler        * m_pScheduler;
	CTimer            * m_pTimer;
#ifdef WITH_WLAN
	CEMMCDevice		    m_EMMC;
	CBcm4343Device    m_WLAN;
	FATFS             m_FileSystem;
#endif
	CNetSubSystem     m_Net;
	CIPAddress        m_DevHttpServerIP;
	CIPAddress        m_PlaygroundHttpServerIP;
#ifdef WITH_WLAN
	CWPASupplicant    m_WPASupplicant;	
#endif
  CDNSClient        m_DNSClient;

	boolean m_useWLAN;
	boolean m_isActive;
	boolean m_isPrepared;
	boolean m_isNetworkInitQueued;
	boolean m_isKernelUpdateQueued;
	boolean m_isFrameQueued;
	boolean m_isSktxKeypressQueued;
	unsigned char m_devServerMessage[32000];
	char * m_networkActionStatusMsg;
	unsigned char * m_sktxScreenContent;
	unsigned char m_sktxScreenContentChunk[2048];
  TMachineModel m_PiModel;
	const char * m_DevHttpHost;
	int m_DevHttpHostPort;
	const char * m_PlaygroundHttpHost;
	int m_PlaygroundHttpHostPort;
	unsigned m_SidekickKernelUpdatePath;
	unsigned m_queueDelay;
	unsigned m_effortsSinceLastEvent;
	unsigned m_skipSktxRefresh;
	unsigned m_sktxScreenPosition;
	unsigned m_sktxReponseLength;
	unsigned m_sktxReponseType;
	unsigned m_sktxKey;
	unsigned m_sktxSession;
	unsigned m_videoFrameCounter;
};

#endif
