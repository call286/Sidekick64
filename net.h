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
#include <circle/sched/scheduler.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/net/netsubsystem.h>

#ifndef _sidekicknet_h
#define _sidekicknet_h

extern CLogger *logger;

class CSidekickNet
{
public:
	CSidekickNet( CInterruptSystem *, CTimer *, CScheduler * );
	~CSidekickNet( void )
	{
	};
	boolean Initialize ( void );
	boolean StoreSidekickKernelFile ( void );
	boolean CheckForSidekickKernelUpdate ( void );
	boolean GetFileViaHTTP (const char * pHost, const char * pFile, char *pBuffer, unsigned & nLengthRead);
	boolean UpdateTime (void);
	void contactDevServer();
	
	CNetConfig * GetNetConfig();
	
private:
	CUSBHCIDevice     m_USBHCI;
	CScheduler			  * m_pScheduler;
	CTimer				    * m_pTimer;
	CNetSubSystem     m_Net;
	boolean m_isActive;
	boolean m_storeFile;
	unsigned m_FileLength;
	char * m_pFileBuffer;
	const char * m_DevHttpHost;
	const char * m_SidekickKernelLocation;
};

#endif
