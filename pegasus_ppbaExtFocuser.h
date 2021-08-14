//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1



#ifndef __PEGASUS_PPBA_EXT_FOC__
#define __PEGASUS_PPBA_EXT_FOC__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

// #define PLUGIN_DEBUG_PPBA_EXT_FOC 2

#define DRIVER_VERSION_FOC     1.16

#define SERIAL_BUFFER_SIZE_FOC 1024
#define MAX_TIMEOUT_FOC 2500
#define MAX_READ_WAIT_TIMEOUT_FOC 25
#define NB_RX_WAIT_FOC 25

#define TEXT_BUFFER_SIZE_FOC    1024

enum Errors_PPBA_EXT_FOC    {PLUGIN_OK_PPBA_EXT_FOC = 0, NOT_CONNECTED_PPBA_EXT_FOC, CANT_CONNECT_PPBA_EXT_FOC, BAD_CMD_RESPONSE_PPBA_EXT_FOC, COMMAND_FAILED_PPBA_EXT_FOC, COMMAND_TIMEOUT_PPBA_EXT_FOC};
enum DeviceType_PPBA_EXT_FOC     {NONE_FOC = 0, PPBA_EXT_FOC};
enum GetLedStatus_PPBA_EXT_FOC   {OFF_FOC = 0, ON_FOC};
enum SetLEdStatus_PPBA_EXT_FOC   {SWITCH_OFF_FOC = 0, SWITCH_ON_FOC};
enum MotorDir_PPBA_EXT_FOC       {NORMAL = 1 , REVERSE = 2};
enum MotorStatus_PPBA_EXT_FOC    {IDLE = 0, MOVING};

class CPegasusPPBA_EXTFocuser
{
public:
    CPegasusPPBA_EXTFocuser();
    ~CPegasusPPBA_EXTFocuser();

    int         Connect(const char *pszPort);
    void        Disconnect(int nInstanceCount);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void        getSerxPointer(SerXInterface *p) { p = m_pSerx; };
    void        setLogger(LoggerInterface *pLogger) { m_pLogger = pLogger; };
    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper;};

    // move commands
    int         haltFocuser();
    int         gotoPosition(int nPos);
    int         moveRelativeToPosision(int nSteps);

    // command complete functions
    int         isGoToComplete(bool &bComplete);
    int         isMotorMoving(bool &bMoving);

    // getter and setter
    int         getStatus();

    int         getMotoMaxSpeed(int &nSpeed);
    int         setMotoMaxSpeed(int nSpeed);

    int         getBacklashComp(int &nSteps);
    int         setBacklashComp(int nSteps);

    int         getFirmwareVersion(std::string &sFirmwareVersion);
    void        getFirmwareString(std::string &sFirmware);

    int         getTemperature(double &dTemperature);

    int         getPosition(int &nPosition);

    int         syncMotorPosition(int nPos);

    int         getDeviceType(int &nDevice);
    void        getDeviceTypeString(std::string &sDeviceType);
    int         getPosLimit(void);
    void        setPosLimit(int nLimit);

    bool        isPosLimitEnabled(void);
    void        enablePosLimit(bool bEnable);

    int         setReverseEnable(bool bEnabled);
    int         getReverseEnable(bool &bEnabled);

    int         getMicrostepping(int &nStepping);
    int         setMicrostepping(int nStepping);

    float       getTemp();


protected:
    int             pppaCommand(const char *pszCmd, std::string &sResp, int nTimeout = MAX_TIMEOUT_FOC);
    int             readResponse(std::string &sResp, int nTimeout = MAX_TIMEOUT_FOC);
    int             parseResp(std::string sResp, std::vector<std::string> &svFields, char cSeparator=':');

    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);

    SerXInterface       *m_pSerx;
    LoggerInterface     *m_pLogger;
    SleeperInterface    *m_pSleeper;

    bool            m_bDebugLog;
    bool            m_bIsConnected;
    std::string     m_sFirmwareVersion;
    int             m_nCurpos;
    int             m_nTargetPos;
    int             m_nPosLimit;
    bool            m_bPosLimitEnabled;
	bool			m_bAbborted;
    int             nDeviceType;
    
	
#ifdef PLUGIN_DEBUG_PPBA_EXT_FOC
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};

#endif //__PEGASUS_C__
