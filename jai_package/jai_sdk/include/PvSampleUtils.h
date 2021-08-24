// *****************************************************************************
//
//     Copyright (c) 2012, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#ifndef __PVSAMPLEUTILS_H__
#define __PVSAMPLEUTILS_H__

#ifdef WIN32

#ifdef _AFXDLL
    #include <afxwin.h>
#else
    #include <windows.h>
#endif // _AFXDLL
#include <process.h>
#include <conio.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' " \
    "name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' " \
    "processorArchitecture='*' " \
    "publicKeyToken='6595b64144ccf1df' " \
    "language='*'\"")

#endif // WIN32

#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>
#include <PvDeviceGEV.h>

using namespace std;

extern volatile bool gStop;

#ifdef _UNIX_

#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

#ifdef __QNXNTO__
#include <pthread.h>
#endif

#ifdef __MACH__
typedef void (*sighandler_t)(int);
#endif

#define PV_INIT_SIGNAL_HANDLER()                                    \
    void CatchCtrlC( int aSig ) { gStop = 1; }                      \
    int InitHandler() { signal( SIGINT, CatchCtrlC ); return 1; }   \
    int gInit = InitHandler();                                      \
    volatile bool gStop = false

#define PV_DISABLE_SIGNAL_HANDLER()                            \
    do {                                                       \
    sighandler_t lOrigHandler = signal( SIGINT, SIG_DFL )

#define PV_ENABLE_SIGNAL_HANDLER()     \
        signal( SIGINT, lOrigHandler); \
    } while( 0 )

#define PV_SAMPLE_INIT()
#define PV_SAMPLE_TERMINATE()

inline int PvKbHit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    if ( gStop )
    {
        return 1;
    }

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

inline int PvGetChar()
{
    if ( gStop )
    {
        return 0;
    }

    return getchar();
}

inline void PvWaitForKeyPress()
{
    while ( !PvKbHit() )
    {
        usleep( 10000 );
    }

    PvGetChar(); // Flush key buffer for next stop
}

inline uint64_t PvGetTickCountMs()

{
    timeval ts;
    gettimeofday( &ts, 0 );

    uint64_t lTickCount = (int64_t)(ts.tv_sec * 1000 + ( ts.tv_usec / 1000 ) );

    return lTickCount;

}

inline void PvSleepMs( uint32_t aSleepTime )
{
    usleep( aSleepTime * 1000 );
}

class PvMutex
{
public:

    PvMutex() { pthread_mutex_init( &mMutex, NULL ); }
    ~PvMutex() { pthread_mutex_destroy( &mMutex ); }

    void Lock() { pthread_mutex_lock( &mMutex ); }
    void Unlock() { pthread_mutex_unlock( &mMutex ); }

private:

    pthread_mutex_t  mMutex;
};

#define PvScanf ( scanf )

#endif // _UNIX_

#ifdef WIN32

#define PV_INIT_SIGNAL_HANDLER()                                                    \
volatile bool gStop = false;                                                        \
                                                                                    \
BOOL ExitHandler( DWORD aEvent )                                                    \
{                                                                                   \
    if ( aEvent == CTRL_CLOSE_EVENT || aEvent == CTRL_C_EVENT )                     \
    {                                                                               \
        gStop = true;                                                               \
        Sleep( 10000 ); /* Give enough time for the process to clean-up */          \
        return TRUE;                                                                \
    }                                                                               \
                                                                                    \
    return FALSE;                                                                   \
}                                                                                   \
                                                                                    \
int gInit = SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ExitHandler, TRUE );

#define PV_DISABLE_SIGNAL_HANDLER() SetConsoleCtrlHandler( NULL, TRUE );

#define PV_ENABLE_SIGNAL_HANDLER() SetConsoleCtrlHandler( NULL, FALSE );

// This macro creates the right context to properly use MFC dialogs from a command line application
#ifdef _AFXDLL
    #define PV_SAMPLE_INIT() \
        CWinApp lApp; \
        AfxWinInit( ::GetModuleHandle( NULL ), NULL, ::GetCommandLine(), 0 );
#else
    #define PV_SAMPLE_INIT()
#endif // _AFXDLL

// Cleans up the context set by PV_SAMPLE_INIT
#ifdef _AFXDLL
    #define PV_SAMPLE_TERMINATE() \
        AfxWinTerm();
#else
    #define PV_SAMPLE_TERMINATE()
#endif // _AFXDLL

inline int PvKbHit( void )
{
    if ( gStop )
    {
        return 1;
    }

    return _kbhit();
}

inline int PvGetChar()
{
    if ( gStop )
    {
        return 0;
    }

    return _getch();
}

inline void PvWaitForKeyPress()
{
    while ( !PvKbHit() )
    {
        ::Sleep( 10 );
    }

    PvGetChar(); // Flush key buffer for next stop.
}

inline uint64_t PvGetTickCountMs()
{
    return ::GetTickCount();
}

inline void PvSleepMs( uint32_t aSleepTime )
{
    ::Sleep( aSleepTime );
}

class PvMutex
{
public:

    PvMutex() { mMutex = ::CreateMutex( NULL, FALSE, NULL ); }
    ~PvMutex() { ::CloseHandle( mMutex ); }

    void Lock() { ::WaitForSingleObject( mMutex, INFINITE ); }
    void Unlock() { ::ReleaseMutex( mMutex ); }

private:

    HANDLE mMutex;
};

#define PvScanf ( scanf_s )

#endif // WIN32

inline void PvFlushKeyboard()
{
    while( PvKbHit() )
    {
        if ( gStop )
        {
            break;
        }

	    PvGetChar();
    }
}


// These defines ensures no old/deprecated pixel types are used in the samples.
#ifndef PV_NO_GEV1X_PIXEL_TYPES
#define PV_NO_GEV1X_PIXEL_TYPES
#endif // PV_NO_GEV1X_PIXEL_TYPES
#ifndef PV_NO_DEPRECATED_PIXEL_TYPES
#define PV_NO_DEPRECATED_PIXEL_TYPES
#endif // PV_NO_DEPRECATED_PIXEL_TYPES


inline bool PvSelectDevice( PvString *aConnectionID, PvDeviceInfoType *aType = NULL )
{
    PvResult lResult;
    const PvDeviceInfo *lSelectedDI = NULL;
    PvSystem lSystem;

    cout << endl << "Detecting devices." << endl;

    for ( ;; )
    {
        if ( gStop )
        { 
            return false;
        }
        lSystem.Find();

        // Detect, select device.
        vector<const PvDeviceInfo *> lDIVector;
        for ( uint32_t i = 0; i < lSystem.GetInterfaceCount(); i++ )
        {
            const PvInterface *lInterface = dynamic_cast<const PvInterface *>( lSystem.GetInterface( i ) );
            if ( lInterface != NULL )
            {
                cout << "   " << lInterface->GetDisplayID().GetAscii() << endl;
                for ( uint32_t j = 0; j < lInterface->GetDeviceCount(); j++ )
                {
                    const PvDeviceInfo *lDI = dynamic_cast<const PvDeviceInfo *>( lInterface->GetDeviceInfo( j ) );
                    if ( lDI != NULL )
                    {
                        lDIVector.push_back( lDI );
                        cout << "[" << ( lDIVector.size() - 1 ) << "]" << "\t" << lDI->GetDisplayID().GetAscii() << endl;
                    }					
                }
            }
        }

        if ( lDIVector.size() == 0)
        {
            cout << "No device found!" << endl;
        }

        cout << "[" << lDIVector.size() << "] to abort" << endl; 
        cout << "[" << ( lDIVector.size() + 1 ) << "] to search again" << endl << endl; 

        cout << "Enter your action or device selection?" << endl;
        cout << ">";

        // Read device selection, optional new IP address.
        uint32_t lIndex = 0;
        PV_DISABLE_SIGNAL_HANDLER();
        cin >> lIndex;
        PV_ENABLE_SIGNAL_HANDLER();
        if ( lIndex == lDIVector.size() )
        {
            // We abort the selection process.
            return false;
        }
        else if ( lIndex < lDIVector.size() )
        {
            // The device is selected
            lSelectedDI = lDIVector[ lIndex ];
            break;
        }
    }

#ifndef WIN32
    // Flush the keyboard buffer so subsequent !kbhit() will work
    PvFlushKeyboard();
#endif // WIN32

    // If the IP Address valid?
    if ( lSelectedDI->IsConfigurationValid() )
    {
        cout << endl;
        *aConnectionID = lSelectedDI->GetConnectionID();
        if ( aType != NULL )
        {
            *aType = lSelectedDI->GetType();
        }

        return true;
    }

    if( ( lSelectedDI->GetType() == PvDeviceInfoTypeUSB ) || ( lSelectedDI->GetType() == PvDeviceInfoTypeU3V ) )
    {
        cout << "This device must be connected to a USB 3.0 (SuperSpeed) port." << endl;
        return false;
    }

    // Ask the user for a new IP address.
    cout << "The IP configuration of the device is not valid." << endl;
    cout << "Which IP address should be assigned to the device?" << endl;
    cout << ">";

    // Read new IP address.
    string lNewIPAddress;
    PV_DISABLE_SIGNAL_HANDLER();
    cin >> lNewIPAddress;
    PV_ENABLE_SIGNAL_HANDLER();
    if ( !lNewIPAddress.length() )
    {
        return false;
    }
    PvFlushKeyboard();

#ifndef WIN32
    // Flush the keyboard buffer so subsequent !kbhit() will work
    PvFlushKeyboard();
#endif // WIN32

    const PvDeviceInfoGEV* lDeviceGEV = dynamic_cast<const PvDeviceInfoGEV *>( lSelectedDI );
    if ( lDeviceGEV != NULL )
    {
        // Force new IP address.
        lResult = PvDeviceGEV::SetIPConfiguration( lDeviceGEV->GetMACAddress().GetAscii(), lNewIPAddress.c_str(),
            lDeviceGEV->GetSubnetMask().GetAscii(), lDeviceGEV->GetDefaultGateway().GetAscii() );
        if ( !lResult.IsOK() )
        {
            cout << "Unable to force new IP address." << endl;
            return false;
        }
    }	

    // Wait for the device to come back on the network.
    int lTimeout;
    for ( ;; )
    {
        if ( gStop )
        { 
            return false;
        }
        lTimeout = 10;
        while( lTimeout )
        {
            if ( gStop )
            { 
                return false;
            }

            lSystem.Find();

            vector<const PvDeviceInfo *> lDIVector;
            for ( uint32_t i = 0; i < lSystem.GetInterfaceCount(); i++ )
            {
                const PvInterface *lInterface = lSystem.GetInterface( i );
                for ( uint32_t j = 0; j < lInterface->GetDeviceCount(); j++ )
                {
                    if ( lInterface->GetDeviceInfo( j )->GetType() == PvDeviceInfoTypeGEV )
                    {
                        const PvDeviceInfoGEV *lDI = dynamic_cast<const PvDeviceInfoGEV*>( lInterface->GetDeviceInfo( j ) );
                        if ( lDI != NULL )
                        {
                            if ( lNewIPAddress == lDI->GetIPAddress().GetAscii() )
                            {
                                cout << endl;
                                *aConnectionID = lDI->GetConnectionID();
                                if ( aType != NULL )
                                {
                                    *aType = lDI->GetType();
                                }

                                return true;
                            }
                        }
                    }
                }
            }
            PvSleepMs( 1000 );

            lTimeout--;
        }

        cout << "The device " << lNewIPAddress << " was not located. Do you want to continue waiting? yes or no" << endl;
        cout << ">";

        string lAnswer;
        PV_DISABLE_SIGNAL_HANDLER();
        cin >> lAnswer;
        PV_ENABLE_SIGNAL_HANDLER();
        if ( ( lAnswer == "n" ) || ( lAnswer == "no" ) )
        {
            break;
        }
    }

    cout << endl;
    return false;
}

inline PvString PvSelectInterface()
{
    PvSystem lSystem;
    uint32_t lCount = lSystem.GetInterfaceCount();
    for ( uint32_t i = 0; i < lCount; i++ )
    {
        const PvNetworkAdapter *lNA = dynamic_cast<const PvNetworkAdapter *>( lSystem.GetInterface( i ) );
        if ( ( lNA == NULL ) ||
            ( lNA->GetIPAddressCount() == 0 ) ||
            ( lNA->GetIPAddress( 0 ) == "0.0.0.0" ) )
        {
            continue;
        }

        std::cout << i << ") " << lNA->GetDisplayID().GetAscii() << std::endl;
    }

    uint32_t lIndex = 0;
    std::cout << "Network interface selection?" << endl;
    std::cout << ">";
    std::cin >> lIndex;

    std::cout << std::endl;

    if ( !std::cin.fail() &&
        ( lIndex >= 0 ) &&
        ( lIndex < lCount ) )
    {
        const PvNetworkAdapter *lNA = dynamic_cast<const PvNetworkAdapter *>( lSystem.GetInterface( lIndex ) );
        if ( lNA != NULL )
        {
            return lNA->GetMACAddress();
        }
    }

    return "";
}

inline bool ParseOptionFlag( int aCount, const char ** aArgs, const char *aOption, bool *aValue = NULL )
{
    string lOption = aOption;
    for ( int i = 1; i < aCount; i++ )
    {
        string lString = aArgs[i];
        size_t lPos = lString.find( aOption );
        if ( lPos != string::npos )
        {
            if ( aValue != NULL )
            {
                *aValue = true;
            }

            return true;
        }
    }    

    return false;
}

template <class T>
inline bool ParseOption( int aCount, const char ** aArgs, const char *aOption, T &aValue )
{
    string lOption = aOption;
    lOption += "=";
    for ( int i = 1; i < aCount; i++ )
    {
        string lString = aArgs[i];

        size_t lPos = lString.find( aOption );
        if ( lPos != string::npos )
        {
            if ( lString.size() > lOption.size() )
            {
                string lParameter = lString.substr( lOption.size(), ( lString.size() - lOption.size() ) + 1 );
                istringstream iss( lParameter, istringstream::in );
                iss >> aValue;
                return true;
            }
        }
    }

    return false;
}


#endif