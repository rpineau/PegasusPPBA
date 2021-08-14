//
//
//  Created by Rodolphe Pineau on 3/11/2020.


#ifndef __PEGASUS_PPBA__
#define __PEGASUS_PPBA__

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

// #define PLUGIN_DEBUG 2

#define SERIAL_BUFFER_SIZE 1024
#define MAX_TIMEOUT 1000
#define TEXT_BUFFER_SIZE    1024

#define DRIVER_VERSION  1.16

#define NB_PORTS 4
#define QUAD12  1
#define ADJ     2

#define PWMA    1
#define PWMB    2

// field indexes in response for PA command
#define ppbVoltage          1
#define ppbCurrent          2
#define ppbTemp             3
#define ppbHumidity         4
#define ppbDewPoint         5
#define ppbQuadPortStatus   6
#define ppbDSLRPortStatus   7
#define ppbDew1PWM          8
#define ppbDew2PWM          9
#define ppbAutodew         10
#define ppbPowerAlert      11
#define ppbAdjPortVolt     12

// field indexes in response for PS command
#define averageAmps 1
#define ampHours    2
#define wattHours   3
#define uptimeMs    4

// field indexes in response for PC command
#define totalAmps   1
#define ampsQuad    2
#define ampsDewA    3
#define ampsDewB    4

enum DMFC_Errors    {PLUGIN_OK = 0, NOT_CONNECTED, ND_CANT_CONNECT, PPB_BAD_CMD_RESPONSE, COMMAND_FAILED};
enum DeviceType     {NONE = 0, PPBA};
enum LedStatus      {OFF = 0, ON};

typedef struct {
    int     nDeviceType;
    bool    bReady;
    char    szVersion[TEXT_BUFFER_SIZE];
    int     nLedStatus;
    float   fVoltage;
    float   fCurent;
    float   fTemp;
    int     nHumidity;
    float   fDewPoint;
    bool    bQuadPortOn;
    bool    bAdjPortOn;
    int     AdjPortVolts;
    int     nDewAPWM;
    int     nDewBPWM;
    bool    bAutoDew;
    bool    bPowerAlert;

    float   fAverageAmps;
    float   fAmpHours;
    float   fWattHours;
    int     nUptimeMs;

    float   fTotalAmps;
    float   fAmpsQuad;
    float   fAmpsDewA;
    float   fAmpsDewB;

} ppbaStatus;




class CPegasusPPBAPower
{
public:
    CPegasusPPBAPower();
    ~CPegasusPPBAPower();

    int         Connect(const char *pszPort);
    void        Disconnect(int nInstanceCount);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };

    // getter and setter
    int         getStatus(int &nStatus);
    int         getConsolidatedStatus(void);
    int         getOnBootPowerState(void);

    int         getFirmwareVersion(char *pszVersion, int nStrMaxLen);
    void        getFirmwareVersionString(std::string sFirmware);
    int         getLedStatus(int &nStatus);
    int         setLedStatus(int nStatus);

    int         getDeviceType(int &nDevice);

    float       getVoltage();
    float       getCurrent();
    float       getTemp();
    int         getHumidity();
    float       getDewPoint();

    bool        getPortOn(const int &nPortNumber);
    int         setPortOn(const int &nPortNumber, const bool &bEnabled);
    bool        getOnBootPortOn(const int &nPortNumber);
    int         setOnBootPortOn(const int &nPortNumber, const bool &bEnable);

    int         setDewHeaterPWM(const int &nDewHeater, const int &nPWM);
    int         getDewHeaterPWM(const int &nDewHeater);
    int         setDewHeaterPWMVal(const int &nDewHeater, const int &nPWM);

    int         setAutoDewOn(const bool &bOn);
    bool        isAutoDewOn();

    int         getAutoDewAggressivness(int &nLevel);
    int         setAutoDewAggressivness(int nLevel);

    int         getAdjVoltage();
    int         setAdjVoltage(int nVolt);

    int         getPortCount();
    
    int         getPower(float &fAverageAmps, float &fAmpHours, float &fWattHours, unsigned long &nUptimeMs);
    int         getPowerData();
    
    int         getPowerMetric(float &fTotalAmps, float &fAmpsQuad, float &fAmpsDewA, float &fAmpsDewB);
    int         getPowerMetricData();

protected:

    int             ppbCommand(const char *pszCmd, char *pszResult, unsigned long nResultMaxLen);
    int             readResponse(char *pszRespBuffer, unsigned long nBufferLen);
    int             parseResp(char *pszResp, std::vector<std::string>  &sParsedRes);



    SerXInterface   *m_pSerx;

    bool            m_bIsConnected;
    char            m_szFirmwareVersion[TEXT_BUFFER_SIZE];

    std::vector<std::string>    m_svParsedResp;

    int             m_nPWMA;
    bool            m_bPWMA_On;

    int             m_nPWMB;
    bool            m_bPWMB_On;

    ppbaStatus      m_globalStatus;
    int             m_nTargetPos;
    int             m_nPosLimit;
    bool            m_bPosLimitEnabled;
	bool			m_bAbborted;
    int             m_nAutoDewAgressiveness;
    
#ifdef PLUGIN_DEBUG
    std::string     m_sLogfilePath;
    // timestamp for logs
    char            *timestamp;
    time_t          ltime;
    FILE            *Logfile;
#endif

};

#endif //__PEGASUS_PPBA__
