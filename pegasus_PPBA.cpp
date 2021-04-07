//
//  Pegasus pocket power box X2 plugin
//
//  Created by Rodolphe Pineau on 3/11/2020.


#include "pegasus_PPBA.h"


CPegasusPPBA::CPegasusPPBA()
{
    m_globalStatus.nDeviceType = NONE;
    m_globalStatus.bReady = false;
    memset(m_globalStatus.szVersion,0,TEXT_BUFFER_SIZE);

    m_nTargetPos = 0;
    m_nPosLimit = 0;
    m_bPosLimitEnabled = false;
    m_bAbborted = false;
    m_bIsConnected = false;
    memset(&m_globalStatus, 0, sizeof(ppbaStatus));
    m_nAutoDewAgressiveness = 210;
    m_bPWMA_On = false;
    m_bPWMB_On = false;
    m_pSerx = NULL;


#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PegasusPPBALog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusPPBALog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusPPBALog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::CPegasusPPBA] build 2021_01_24_1540.\n", timestamp);
    fprintf(Logfile, "[%s] [CPegasusPPBA::CPegasusPPBA] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusPPBA::~CPegasusPPBA()
{
#ifdef	PLUGIN_DEBUG
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusPPBA::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK;
    int nDevice;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif

    // 9600 8N1
    nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
    if(nErr == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return nErr;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA::Connect getting device type\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != PPBA) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusPPBA::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    
    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusPPBA::Connect **** ERROR **** getting device full status\n", timestamp);
        fflush(Logfile);
#endif
    }
    m_nPWMA = m_globalStatus.nDewAPWM;
    m_nPWMB = m_globalStatus.nDewBPWM;
    m_bPWMA_On = (m_nPWMA!=0);
    m_bPWMB_On = (m_nPWMB!=0);

    return nErr;
}

void CPegasusPPBA::Disconnect()
{
    if(m_bIsConnected && m_pSerx)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark getters and setters
int CPegasusPPBA::getStatus(int &nStatus)
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    // OK_ppb or OK_PPB
    nErr = ppbCommand("P#\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    if(strstr(szResp,"_OK")) {
        if(strstr(szResp,"PPBA")) {
            m_globalStatus.nDeviceType = PPBA;
        }
        else {
            nStatus = PPB_BAD_CMD_RESPONSE;
            m_globalStatus.nDeviceType = NONE;
            return ERR_DEVICENOTSUPPORTED;
        }
        nStatus = PLUGIN_OK;
        nErr = PLUGIN_OK;
    }
    else {
        nErr = COMMAND_FAILED;
        nStatus = PPB_BAD_CMD_RESPONSE;
    }
    return nErr;
}


int CPegasusPPBA::getConsolidatedStatus()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = ppbCommand("PA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus]  about to parse response\n", timestamp);
	fflush(Logfile);
#endif

    // parse response
    nErr = parseResp(szResp, m_svParsedResp);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus]  response parsing done\n", timestamp);
	fflush(Logfile);
#endif

    if(m_svParsedResp.size()<13) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] converting value and setting them in m_globalStatus\n", timestamp);
    fflush(Logfile);
#endif

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbVoltage] = %s \n", timestamp, m_svParsedResp[ppbVoltage].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbCurrent] = %s \n", timestamp, m_svParsedResp[ppbCurrent].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbTemp] = %s \n", timestamp, m_svParsedResp[ppbTemp].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbHumidity] = %s \n", timestamp, m_svParsedResp[ppbHumidity].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbDewPoint] = %s \n", timestamp, m_svParsedResp[ppbDewPoint].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbQuadPortStatus] = %s \n", timestamp, m_svParsedResp[ppbQuadPortStatus].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbDSLRPortStatus] = %s \n", timestamp, m_svParsedResp[ppbDSLRPortStatus].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbDew1PWM] = %s \n", timestamp, m_svParsedResp[ppbDew1PWM].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbDew2PWM] = %s \n", timestamp, m_svParsedResp[ppbDew2PWM].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbAutodew] = %s \n", timestamp, m_svParsedResp[ppbAutodew].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbPowerAlert] = %s \n", timestamp, m_svParsedResp[ppbPowerAlert].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBA::getConsolidatedStatus] m_svParsedResp[ppbAdjPortVolt] = %s \n", timestamp, m_svParsedResp[ppbAdjPortVolt].c_str());
    fflush(Logfile);
#endif

    m_globalStatus.fVoltage = std::stof(m_svParsedResp[ppbVoltage]);
    m_globalStatus.fCurent = std::stof(m_svParsedResp[ppbCurrent]);
    m_globalStatus.fTemp = std::stof(m_svParsedResp[ppbTemp]);
    m_globalStatus.nHumidity = std::stoi(m_svParsedResp[ppbHumidity]);
    m_globalStatus.fDewPoint = std::stof(m_svParsedResp[ppbDewPoint]);
    m_globalStatus.bQuadPortOn = std::stoi(m_svParsedResp[ppbQuadPortStatus])==1;
    m_globalStatus.bAdjPortOn = std::stoi(m_svParsedResp[ppbDSLRPortStatus])==1;
    m_globalStatus.nDewAPWM = std::stoi(m_svParsedResp[ppbDew1PWM]);
    m_globalStatus.nDewBPWM = std::stoi(m_svParsedResp[ppbDew2PWM]);
    m_globalStatus.bAutoDew = std::stoi(m_svParsedResp[ppbAutodew])==1;
    m_globalStatus.bPowerAlert = std::stoi(m_svParsedResp[ppbPowerAlert])==1;
    m_globalStatus.AdjPortVolts = std::stof(m_svParsedResp[ppbAdjPortVolt]);

    

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] fVoltage = %3.2f\n", timestamp, m_globalStatus.fVoltage);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] fCurent = %3.2f\n", timestamp, m_globalStatus.fCurent);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] fTemp = %3.2f\n", timestamp, m_globalStatus.fTemp);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] nHumidity = %d\n", timestamp, m_globalStatus.nHumidity);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] fDewPoint = %3.2f\n", timestamp, m_globalStatus.fDewPoint);

    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] bQuadPortOn = %s\n", timestamp, m_globalStatus.bQuadPortOn?"On":"Off");
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] bAdjPortOn = %s\n", timestamp, m_globalStatus.bAdjPortOn?"On":"Off");

    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] nDewAPWM = %d\n", timestamp, m_globalStatus.nDewAPWM);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] nDewBPWM = %d\n", timestamp, m_globalStatus.nDewBPWM);
    
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] bAutoDew = %s\n", timestamp, m_globalStatus.bAutoDew?"On":"Off");
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] bPowerAlert = %s\n", timestamp, m_globalStatus.bPowerAlert?"Alert":"Ok");
    fprintf(Logfile, "[%s] [CPegasusPPBA::getConsolidatedStatus] AdjPortVolts = %d\n", timestamp, m_globalStatus.AdjPortVolts);


    fflush(Logfile);
#endif

    
    return nErr;
}


int CPegasusPPBA::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = ppbCommand("PV\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

int CPegasusPPBA::getLedStatus(int &nStatus)
{
    int nErr = PLUGIN_OK;
    int nLedStatus = 0;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = ppbCommand("PL\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    nErr = parseResp(szResp, sParsedResp);
    nLedStatus = atoi(sParsedResp[1].c_str());
    switch(nLedStatus) {
        case 0:
            nStatus = OFF;
            break;
        case 1:
            nStatus = ON;
            break;
    }

    return nErr;
}

int CPegasusPPBA::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", nStatus);
    nErr = ppbCommand(szCmd, NULL, 0);

    return nErr;
}


int CPegasusPPBA::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK;
    int nStatus;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus(nStatus);
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}


float CPegasusPPBA::getVoltage()
{
    return m_globalStatus.fVoltage;
}

float CPegasusPPBA::getCurrent()
{
    getPowerData();
    return m_globalStatus.fAverageAmps;
}

float CPegasusPPBA::getTemp()
{
    return m_globalStatus.fTemp;
}

int CPegasusPPBA::getHumidity()
{
    return m_globalStatus.nHumidity;
}

float CPegasusPPBA::getDewPoint()
{
    return m_globalStatus.fDewPoint;

}

bool CPegasusPPBA::getPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.bQuadPortOn;
            break;

        case 2:
            return m_globalStatus.bAdjPortOn;
            break;

        case 3:
            if(m_globalStatus.bAutoDew)
                return true;
            return m_bPWMA_On;
            break;

        case 4:
            if(m_globalStatus.bAutoDew)
                return true;
            return m_bPWMB_On;
            break;

        default:
            return false;
            break;
    }
}

int CPegasusPPBA::setPortOn(const int &nPortNumber, const bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::setPortOn] nPortNumber = %d\n", timestamp, nPortNumber);
    fprintf(Logfile, "[%s] [CPegasusPPBA::setPortOn] bEnabled = %s\n", timestamp, bEnabled?"On":"Off");
    fflush(Logfile);
#endif

    switch(nPortNumber) {
        case 1:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P1:%d\n", bEnabled?1:0);
            m_globalStatus.bQuadPortOn = bEnabled;
            nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
            break;

        case 2:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P2:%d\n", bEnabled?1:0);
            m_globalStatus.bAdjPortOn = bEnabled;
            nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
            break;

        case 3:
            if(m_globalStatus.bAutoDew) {
                return nErr;
            }
            m_bPWMA_On = bEnabled;
            nErr = setDewHeaterPWM(PWMA, bEnabled?m_nPWMA:0);
            break;

        case 4:
            if(m_globalStatus.bAutoDew) {
                return nErr;
            }
            m_bPWMB_On = bEnabled;
            nErr = setDewHeaterPWM(PWMB, bEnabled?m_nPWMB:0);
            break;

        default:
            nErr = ERR_CMDFAILED;
            break;
    }

    return nErr;
}


int CPegasusPPBA::getOnBootPowerState()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;
    
    if(!m_bIsConnected)
        return ERR_COMMNOLINK;
    
    // get boot power state for the quad port and adjustable port
    nErr = ppbCommand("PE:99\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    
    if(strlen(szResp)==3) {
        m_globalStatus.bQuadPortOn = false;
        m_globalStatus.bAdjPortOn = szResp[0] == '1'? true : false;

    }
    else {
        m_globalStatus.bQuadPortOn = szResp[0] == '1'? true : false;
        m_globalStatus.bAdjPortOn = szResp[1] == '1'? true : false;
    }
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::getOnBootPowerState] bQuadPortOn = %s\n", timestamp, m_globalStatus.bQuadPortOn?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusPPBA::getOnBootPowerState] bAdjPortOn = %s\n", timestamp, m_globalStatus.bAdjPortOn?"Yes":"No");
    fflush(Logfile);
#endif
    
    return nErr;
}

bool CPegasusPPBA::getOnBootPortOn(const int &nPortNumber)
{
    getOnBootPowerState();
    
    switch(nPortNumber) {
        case QUAD12:
            return m_globalStatus.bQuadPortOn;
            break;

        case ADJ:
            return m_globalStatus.bAdjPortOn;
            break;

        default:
            return false;
            break;
    }
}


int CPegasusPPBA::setOnBootPortOn(const int &nPortNumber, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    switch(nPortNumber) {
        case QUAD12:
            m_globalStatus.bQuadPortOn = bEnable;
            break;
            
        case ADJ:
            m_globalStatus.bAdjPortOn = bEnable;
            break;
            
        default:
            break;
    }

    sPorts.empty();
    sPorts += m_globalStatus.bQuadPortOn? "1" : "0";
    sPorts += m_globalStatus.bAdjPortOn? "1" : "0";
    sPorts += "0";
    sPorts += "0";

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PE:%s\n", sPorts.c_str());
    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    
    return nErr;
}


int CPegasusPPBA::setDewHeaterPWM(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    switch(nDewHeater) {
        case PWMA:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P3:%d\n", nPWM);
            m_globalStatus.nDewAPWM = nPWM;
            break;
        case PWMB:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P4:%d\n", nPWM);
            m_globalStatus.nDewBPWM = nPWM;
            break;
        default:
            return ERR_CMDFAILED;
            break;
    }

    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

int CPegasusPPBA::getDewHeaterPWM(const int &nDewHeater)
{
    switch(nDewHeater) {
        case PWMA:
            return m_nPWMA  ;
            break;
        case PWMB:
            return m_nPWMB;
            break;
        default:
            return -1;
            break;
    }
}

int CPegasusPPBA::setDewHeaterPWMVal(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK;
    bool bOn = false;
    
    switch(nDewHeater) {
        case PWMA:
            m_nPWMA = nPWM;
            bOn = m_bPWMA_On;
            break;
        case PWMB:
            m_nPWMB = nPWM;
            bOn = m_bPWMB_On;
            break;
        default:
            break;
    }
    if(bOn)
        nErr = setDewHeaterPWM(nDewHeater, nPWM);

    return nErr;
}


int CPegasusPPBA::setAutoDewOn(const bool &bOn)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PD:%s\n", bOn?"1":"0");
    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    m_globalStatus.bAutoDew = bOn;
    m_bPWMA_On = bOn;
    m_bPWMB_On = bOn;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusPPBA::setAutoDewOn  m_globalStatus.bAutoDew : %s\n", timestamp, m_globalStatus.bAutoDew?"Yes":"No");
    fflush(Logfile);
#endif
    getConsolidatedStatus();
    return nErr;
}


bool CPegasusPPBA::isAutoDewOn()
{
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusPPBA::isAutoDewOn  m_globalStatus.bAutoDew : %s\n", timestamp, m_globalStatus.bAutoDew?"Yes":"No");
    fflush(Logfile);
#endif
    return m_globalStatus.bAutoDew;
}

int CPegasusPPBA::getAutoDewAggressivness(int &nLevel)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;
    
    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    
    m_nAutoDewAgressiveness = 210; // default value
    nErr = ppbCommand("DA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    
    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>1) {
        m_nAutoDewAgressiveness = std::stoi(sParsedResp[1]);
    }
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    //fprintf(Logfile, "[%s] [CPegasusPPBA::getAutoDewAggressivness] sParsedResp[1] : %s\n", timestamp, sParsedResp[1].c_str());
    fprintf(Logfile, "[%s] [CPegasusPPBA::getAutoDewAggressivness] m_nAutoDewAgressiveness : %d\n", timestamp, m_nAutoDewAgressiveness);
    fflush(Logfile);
#endif

    nLevel = m_nAutoDewAgressiveness;
    return nErr;
}

int CPegasusPPBA::setAutoDewAggressivness(int nLevel)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PD:%d\n", nLevel);
    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    // setting auto dew agressiveness also enable auto-dew
    m_globalStatus.bAutoDew = true;
    m_bPWMA_On = true;
    m_bPWMB_On = true;
    return nErr;
}

int CPegasusPPBA::getAdjVoltage()
{
    return m_globalStatus.AdjPortVolts;
}

int CPegasusPPBA::setAdjVoltage(int nVolt)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "P2:%d\n", nVolt);
    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    m_globalStatus.AdjPortVolts = nVolt;
    return nErr;
}

int CPegasusPPBA::getPortCount()
{
    return NB_PORTS;
}

int CPegasusPPBA::getPower(float &fAverageAmps, float &fAmpHours, float &fWattHours, unsigned long &nUptimeMs)
{
    int nErr = PLUGIN_OK;
    
    nErr = getPowerData();
    if(nErr)
        return nErr;

    fAverageAmps = m_globalStatus.fAverageAmps;
    fAmpHours = m_globalStatus.fAmpHours;
    fWattHours =m_globalStatus.fWattHours;
    
    return nErr;
}

int CPegasusPPBA::getPowerMetric(float &fTotalAmps, float &fAmpsQuad, float &fAmpsDewA, float &fAmpsDewB)
{
    int nErr = PLUGIN_OK;
    
    nErr = getPowerMetricData();
    if(nErr)
        return nErr;
    fTotalAmps = m_globalStatus.fTotalAmps;
    fAmpsQuad = m_globalStatus.fAmpsQuad;
    fAmpsDewA = m_globalStatus.fAmpsDewA;
    fAmpsDewB = m_globalStatus.fAmpsDewB ;

    return nErr;
}

int CPegasusPPBA::getPowerData()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
    
    if(!m_bIsConnected)
        return ERR_COMMNOLINK;
    
    nErr = ppbCommand("PS\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
        
    // parse response
    nErr = parseResp(szResp, m_svParsedResp);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    if(m_svParsedResp.size()<5) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }
    
    m_globalStatus.fAverageAmps = std::stof(m_svParsedResp[averageAmps]);
    m_globalStatus.fAmpHours = std::stof(m_svParsedResp[ampHours]);
    m_globalStatus.fWattHours = std::stof(m_svParsedResp[wattHours]);
    m_globalStatus.nUptimeMs = std::stoi(m_svParsedResp[uptimeMs]);
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fAverageAmps = %3.2f\n", timestamp, m_globalStatus.fAverageAmps);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fAmpHours = %3.2f\n", timestamp, m_globalStatus.fAmpHours);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fWattHours = %3.2f\n", timestamp, m_globalStatus.fWattHours);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] nUptimeMs = %d\n", timestamp, m_globalStatus.nUptimeMs);
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusPPBA::getPowerMetricData()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
    
    if(!m_bIsConnected)
        return ERR_COMMNOLINK;
    
    
    nErr = ppbCommand("PC\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    // parse response
    nErr = parseResp(szResp, m_svParsedResp);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    if(m_svParsedResp.size()<6) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA::getPower]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }
    
    m_globalStatus.fTotalAmps = std::stof(m_svParsedResp[totalAmps]);
    m_globalStatus.fAmpsQuad = std::stof(m_svParsedResp[ampsQuad]);
    m_globalStatus.fAmpsDewA = std::stof(m_svParsedResp[ampsDewA]);
    m_globalStatus.fAmpsDewB = std::stof(m_svParsedResp[ampsDewB]);
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fTotalAmps = %3.2f\n", timestamp, m_globalStatus.fTotalAmps);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fAmpsQuad = %3.2f\n", timestamp, m_globalStatus.fAmpsQuad);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fAmpsDewA = %3.2f\n", timestamp, m_globalStatus.fAmpsDewA);
    fprintf(Logfile, "[%s] [CPegasusPPBA::getPower] fAmpsDewB = %3.2f\n", timestamp, m_globalStatus.fAmpsDewB);
    fflush(Logfile);
#endif
    
    return nErr;
}


#pragma mark command and response functions

int CPegasusPPBA::ppbCommand(const char *pszszCmd, char *pszResult, unsigned long nResultMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA::ppbCommand Sending %s\n", timestamp, pszszCmd);
	fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszszCmd, strlen(pszszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    if(nErr){
        return nErr;
    }

    if(pszResult) {
        // read response
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusPPBA::ppbCommand response \"%s\"\n", timestamp, szResp);
        fflush(Logfile);
#endif
        if(nErr){
            return nErr;
        }
        strncpy(pszResult, szResp, nResultMaxLen);
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusPPBA::ppbCommand response copied to pszResult : \"%s\"\n", timestamp, pszResult);
		fflush(Logfile);
#endif
    }
    return nErr;
}

int CPegasusPPBA::readResponse(char *pszRespBuffer, unsigned long nBufferLen)
{
    int nErr = PLUGIN_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    memset(pszRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = pszRespBuffer;

    do {
        nErr = m_pSerx->readFile(pszBufPtr, 1, ulBytesRead, MAX_TIMEOUT);
        if(nErr) {
            return nErr;
        }

        if (ulBytesRead !=1) {// timeout
#ifdef PLUGIN_DEBUG
			ltime = time(NULL);
			timestamp = asctime(localtime(&ltime));
			timestamp[strlen(timestamp) - 1] = 0;
			fprintf(Logfile, "[%s] CPegasusPPBA::readResponse timeout\n", timestamp);
			fflush(Logfile);
#endif
            nErr = ERR_NORESPONSE;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
    } while (ulTotalBytesRead < nBufferLen && *pszBufPtr++ != '\n' );

    if(ulTotalBytesRead) {
        *(pszBufPtr-1) = 0; //remove the \n
        *(pszBufPtr-2) = 0; //remove the \r
    }
    return nErr;
}


int CPegasusPPBA::parseResp(char *pszResp, std::vector<std::string>  &svParsedResp)
{
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA::parseResp parsing \"%s\"\n", timestamp, pszResp);
	fflush(Logfile);
#endif
	svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusPPBA::parseResp sSegment : %s\n", timestamp, sSegment.c_str());
        fflush(Logfile);
#endif
    }

    svParsedResp = svSeglist;


    return PLUGIN_OK;
}
