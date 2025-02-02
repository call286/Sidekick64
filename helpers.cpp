/*
  _________.__    .___      __   .__        __        _________   ________   _____  
 /   _____/|__| __| _/____ |  | _|__| ____ |  | __    \_   ___ \ /  _____/  /  |  | 
 \_____  \ |  |/ __ |/ __ \|  |/ /  |/ ___\|  |/ /    /    \  \//   __  \  /   |  |_
 /        \|  / /_/ \  ___/|    <|  \  \___|    <     \     \___\  |__\  \/    ^   /
/_______  /|__\____ |\___  >__|_ \__|\___  >__|_ \     \______  /\_____  /\____   | 
        \/         \/    \/     \/       \/     \/            \/       \/      |__| 
 
 helpers.cpp

 RasPiC64 - A framework for interfacing the C64 and a Raspberry Pi 3B/3B+
          - misc code
 Copyright (c) 2019, 2020 Carsten Dachsbacher <frenetic@dachsbacher.de>

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
#include "helpers.h"

// file reading
int readFile( CLogger *logger, const char *DRIVE, const char *FILENAME, u8 *data, u32 *size )
{
#ifndef WITH_NET
	FATFS m_FileSystem;

	// mount file system
	if ( f_mount( &m_FileSystem, DRIVE, 1 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot mount drive: %s", DRIVE );
#endif

	// get filesize
	FILINFO info;
	u32 result = f_stat( FILENAME, &info );
	u32 filesize = (u32)info.fsize;

	// open file
	FIL file;
	result = f_open( &file, FILENAME, FA_READ | FA_OPEN_EXISTING );
	if ( result != FR_OK )
	{
		logger->Write( "RaspiMenu", LogNotice, "Cannot open file: %s", FILENAME );

#ifndef WITH_NET
		if ( f_mount( 0, DRIVE, 0 ) != FR_OK )
			logger->Write( "RaspiMenu", LogPanic, "Cannot unmount drive: %s", DRIVE );
#endif

		return 0;
	}

	*size = filesize;

	// read data in one big chunk
	u32 nBytesRead;
	result = f_read( &file, data, filesize, &nBytesRead );

	if ( result != FR_OK )
		logger->Write( "RaspiMenu", LogError, "Read error" );

	if ( f_close( &file ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot close file" );

#ifndef WITH_NET
	// unmount file system
	if ( f_mount( 0, DRIVE, 0 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot unmount drive: %s", DRIVE );
#endif
	
	return 1;
}

int getFileSize( CLogger *logger, const char *DRIVE, const char *FILENAME, u32 *size )
{
	#ifndef WITH_NET
	
	FATFS m_FileSystem;

	// mount file system
	if ( f_mount( &m_FileSystem, DRIVE, 1 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot mount drive: %s", DRIVE );
	#endif

	// get filesize
	FILINFO info;
	u32 result = f_stat( FILENAME, &info );

	if ( result != FR_OK )
		return 0;

	*size = (u32)info.fsize;

	#ifndef WITH_NET
	// unmount file system
	if ( f_mount( 0, DRIVE, 0 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot unmount drive: %s", DRIVE );
	#endif
	
	return 1;
}


// file writing
int writeFile( CLogger *logger, const char *DRIVE, const char *FILENAME, u8 *data, u32 size )
{
	
#ifndef WITH_NET
	FATFS m_FileSystem;

	// mount file system
	if ( f_mount( &m_FileSystem, DRIVE, 1 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot mount drive: %s", DRIVE );
#endif
	// open file
	FIL file;
	u32 result = f_open( &file, FILENAME, FA_WRITE | FA_CREATE_ALWAYS );
	if ( result != FR_OK )
	{
		logger->Write( "RaspiMenu", LogNotice, "Cannot open file: %s", FILENAME );
		return 0;
	}

	// write data in one big chunk
	u32 nBytesWritten;
	result = f_write( &file, data, size, &nBytesWritten );

	if ( result != FR_OK )
		logger->Write( "RaspiMenu", LogError, "Write error" );

	if ( f_close( &file ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot close file" );

#ifndef WITH_NET
	// unmount file system
	if ( f_mount( 0, DRIVE, 0 ) != FR_OK )
		logger->Write( "RaspiMenu", LogPanic, "Cannot unmount drive: %s", DRIVE );
#endif
	
	return 1;
}
