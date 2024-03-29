//
//  Pegasus pocket power box X2 plugin
//
//  Created by Rodolphe Pineau on 3/11/2020.


#include "pegasus_PPBAPower.h"


CPegasusPPBAPower::CPegasusPPBAPower()
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
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::CPegasusPPBAPower] version %3.2f build %s %s .\n", timestamp, DRIVER_VERSION, __DATE__, __TIME__);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::CPegasusPPBAPower] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusPPBAPower::~CPegasusPPBAPower()
{
#ifdef	PLUGIN_DEBUG
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusPPBAPower::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK;
    int nDevice;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBAPower::Connect Called %s\n", timestamp, pszPort);
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
	fprintf(Logfile, "[%s] CPegasusPPBAPower::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBAPower::Connect getting device type\n", timestamp);
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
		fprintf(Logfile, "[%s] CPegasusPPBAPower::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }

    getFirmwareVersion(m_szFirmwareVersion, TEXT_BUFFER_SIZE);
    
    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusPPBAPower::Connect **** ERROR **** getting device full status\n", timestamp);
        fflush(Logfile);
#endif
    }
    m_nPWMA = m_globalStatus.nDewAPWM;
    m_nPWMB = m_globalStatus.nDewBPWM;
    m_bPWMA_On = (m_nPWMA!=0);
    m_bPWMB_On = (m_nPWMB!=0);

    return nErr;
}

void CPegasusPPBAPower::Disconnect(int nInstanceCount)
{
    if(m_bIsConnected && m_pSerx && nInstanceCount==1)
        m_pSerx->close();

    m_bIsConnected = false;
}

#pragma mark getters and setters
int CPegasusPPBAPower::getStatus(int &nStatus)
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
        else if(strstr(szResp,"PPBM")) {
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


int CPegasusPPBAPower::getConsolidatedStatus()
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
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus]  about to parse response\n", timestamp);
	fflush(Logfile);
#endif

    // parse response
    nErr = parseResp(szResp, m_svParsedResp);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus]  response parsing done\n", timestamp);
	fflush(Logfile);
#endif

    if(m_svParsedResp.size()<13) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] converting value and setting them in m_globalStatus\n", timestamp);
    fflush(Logfile);
#endif

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbVoltage] = %s \n", timestamp, m_svParsedResp[ppbVoltage].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbCurrent] = %s \n", timestamp, m_svParsedResp[ppbCurrent].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbTemp] = %s \n", timestamp, m_svParsedResp[ppbTemp].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbHumidity] = %s \n", timestamp, m_svParsedResp[ppbHumidity].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbDewPoint] = %s \n", timestamp, m_svParsedResp[ppbDewPoint].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbQuadPortStatus] = %s \n", timestamp, m_svParsedResp[ppbQuadPortStatus].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbDSLRPortStatus] = %s \n", timestamp, m_svParsedResp[ppbDSLRPortStatus].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbDew1PWM] = %s \n", timestamp, m_svParsedResp[ppbDew1PWM].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbDew2PWM] = %s \n", timestamp, m_svParsedResp[ppbDew2PWM].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbAutodew] = %s \n", timestamp, m_svParsedResp[ppbAutodew].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbPowerAlert] = %s \n", timestamp, m_svParsedResp[ppbPowerAlert].c_str());
    fprintf(Logfile, "[%s][CPegasusPPBAPower::getConsolidatedStatus] m_svParsedResp[ppbAdjPortVolt] = %s \n", timestamp, m_svParsedResp[ppbAdjPortVolt].c_str());
    fflush(Logfile);
#endif
    try {
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
    }
    catch(const std::exception& e) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] convertsion exception = %s\n", timestamp, e.what());
#endif
    }

    

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] fVoltage = %3.2f\n", timestamp, m_globalStatus.fVoltage);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] fCurent = %3.2f\n", timestamp, m_globalStatus.fCurent);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] fTemp = %3.2f\n", timestamp, m_globalStatus.fTemp);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] nHumidity = %d\n", timestamp, m_globalStatus.nHumidity);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] fDewPoint = %3.2f\n", timestamp, m_globalStatus.fDewPoint);

    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] bQuadPortOn = %s\n", timestamp, m_globalStatus.bQuadPortOn?"On":"Off");
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] bAdjPortOn = %s\n", timestamp, m_globalStatus.bAdjPortOn?"On":"Off");

    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] nDewAPWM = %d\n", timestamp, m_globalStatus.nDewAPWM);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] nDewBPWM = %d\n", timestamp, m_globalStatus.nDewBPWM);
    
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] bAutoDew = %s\n", timestamp, m_globalStatus.bAutoDew?"On":"Off");
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] bPowerAlert = %s\n", timestamp, m_globalStatus.bPowerAlert?"Alert":"Ok");
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getConsolidatedStatus] AdjPortVolts = %d\n", timestamp, m_globalStatus.AdjPortVolts);


    fflush(Logfile);
#endif

    
    return nErr;
}


int CPegasusPPBAPower::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
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

void CPegasusPPBAPower::getFirmwareVersionString(std::string sFirmware)
{
    sFirmware.assign(m_szFirmwareVersion);
}

int CPegasusPPBAPower::getLedStatus(int &nStatus)
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

int CPegasusPPBAPower::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", nStatus);
    nErr = ppbCommand(szCmd, NULL, 0);

    return nErr;
}


int CPegasusPPBAPower::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK;
    int nStatus;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus(nStatus);
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}


float CPegasusPPBAPower::getVoltage()
{
    return m_globalStatus.fVoltage;
}

float CPegasusPPBAPower::getCurrent()
{
    getPowerData();
    return m_globalStatus.fAverageAmps;
}

float CPegasusPPBAPower::getTemp()
{
    return m_globalStatus.fTemp;
}

int CPegasusPPBAPower::getHumidity()
{
    return m_globalStatus.nHumidity;
}

float CPegasusPPBAPower::getDewPoint()
{
    return m_globalStatus.fDewPoint;

}

bool CPegasusPPBAPower::getPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            getConsolidatedStatus();
           return m_globalStatus.bQuadPortOn;
            break;

        case 2:
            getConsolidatedStatus();
            return m_globalStatus.bAdjPortOn;
            break;

        case 3:
            getConsolidatedStatus();
            if(m_globalStatus.bAutoDew)
                return true;
            return m_bPWMA_On;
            break;

        case 4:
            getConsolidatedStatus();
            if(m_globalStatus.bAutoDew)
                return true;
            return m_bPWMB_On;
            break;

        default:
            return false;
            break;
    }
}

int CPegasusPPBAPower::setPortOn(const int &nPortNumber, const bool &bEnabled)
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
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::setPortOn] nPortNumber = %d\n", timestamp, nPortNumber);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::setPortOn] bEnabled = %s\n", timestamp, bEnabled?"On":"Off");
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

    getConsolidatedStatus();
    return nErr;
}


int CPegasusPPBAPower::getOnBootPowerState()
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
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getOnBootPowerState] bQuadPortOn = %s\n", timestamp, m_globalStatus.bQuadPortOn?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getOnBootPowerState] bAdjPortOn = %s\n", timestamp, m_globalStatus.bAdjPortOn?"Yes":"No");
    fflush(Logfile);
#endif
    
    return nErr;
}

bool CPegasusPPBAPower::getOnBootPortOn(const int &nPortNumber)
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


int CPegasusPPBAPower::setOnBootPortOn(const int &nPortNumber, const bool &bEnable)
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

    sPorts.clear();
    sPorts += m_globalStatus.bQuadPortOn? "1" : "0";
    sPorts += m_globalStatus.bAdjPortOn? "1" : "0";
    sPorts += "0";
    sPorts += "0";

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PE:%s\n", sPorts.c_str());
    nErr = ppbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    
    return nErr;
}


int CPegasusPPBAPower::setDewHeaterPWM(const int &nDewHeater, const int &nPWM)
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

int CPegasusPPBAPower::getDewHeaterPWM(const int &nDewHeater)
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

int CPegasusPPBAPower::setDewHeaterPWMVal(const int &nDewHeater, const int &nPWM)
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


int CPegasusPPBAPower::setAutoDewOn(const bool &bOn)
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
    fprintf(Logfile, "[%s] CPegasusPPBAPower::setAutoDewOn  m_globalStatus.bAutoDew : %s\n", timestamp, m_globalStatus.bAutoDew?"Yes":"No");
    fflush(Logfile);
#endif
    getConsolidatedStatus();
    return nErr;
}


bool CPegasusPPBAPower::isAutoDewOn()
{
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusPPBAPower::isAutoDewOn  m_globalStatus.bAutoDew : %s\n", timestamp, m_globalStatus.bAutoDew?"Yes":"No");
    fflush(Logfile);
#endif
    return m_globalStatus.bAutoDew;
}

int CPegasusPPBAPower::getAutoDewAggressivness(int &nLevel)
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
        try {
            m_nAutoDewAgressiveness = std::stoi(sParsedResp[1]);
        }
        catch(const std::exception& e) {
#ifdef PLUGIN_DEBUG
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusPPBAPower::getAutoDewAggressivness] convertsion exception = %s\n", timestamp, e.what());
#endif
        }
    }
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getAutoDewAggressivness] m_nAutoDewAgressiveness : %d\n", timestamp, m_nAutoDewAgressiveness);
    fflush(Logfile);
#endif

    nLevel = m_nAutoDewAgressiveness;
    return nErr;
}

int CPegasusPPBAPower::setAutoDewAggressivness(int nLevel)
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

int CPegasusPPBAPower::getAdjVoltage()
{
    return m_globalStatus.AdjPortVolts;
}

int CPegasusPPBAPower::setAdjVoltage(int nVolt)
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

int CPegasusPPBAPower::getPortCount()
{
    return NB_PORTS;
}

int CPegasusPPBAPower::getPower(float &fAverageAmps, float &fAmpHours, float &fWattHours, unsigned long &nUptimeMs)
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

int CPegasusPPBAPower::getPowerMetric(float &fTotalAmps, float &fAmpsQuad, float &fAmpsDewA, float &fAmpsDewB)
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

int CPegasusPPBAPower::getPowerData()
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
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] ERROR = %d\n", timestamp, nErr);
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
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    if(m_svParsedResp.size()<5) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }
    try {
        m_globalStatus.fAverageAmps = std::stof(m_svParsedResp[averageAmps]);
        m_globalStatus.fAmpHours = std::stof(m_svParsedResp[ampHours]);
        m_globalStatus.fWattHours = std::stof(m_svParsedResp[wattHours]);
        m_globalStatus.nUptimeMs = std::stoi(m_svParsedResp[uptimeMs]);
    }
    catch(const std::exception& e) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] convertsion exception = %s\n", timestamp, e.what());
#endif
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fAverageAmps = %3.2f\n", timestamp, m_globalStatus.fAverageAmps);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fAmpHours = %3.2f\n", timestamp, m_globalStatus.fAmpHours);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fWattHours = %3.2f\n", timestamp, m_globalStatus.fWattHours);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] nUptimeMs = %d\n", timestamp, m_globalStatus.nUptimeMs);
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusPPBAPower::getPowerMetricData()
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
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetricData] ERROR = %d\n", timestamp, nErr);
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
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetricData] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    
    if(m_svParsedResp.size()<6) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetricData]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return PPB_BAD_CMD_RESPONSE;
    }
    try {
        m_globalStatus.fTotalAmps = std::stof(m_svParsedResp[totalAmps]);
        m_globalStatus.fAmpsQuad = std::stof(m_svParsedResp[ampsQuad]);
        m_globalStatus.fAmpsDewA = std::stof(m_svParsedResp[ampsDewA]);
        m_globalStatus.fAmpsDewB = std::stof(m_svParsedResp[ampsDewB]);
    }
    catch(const std::exception& e) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetricData] convertsion exception = %s\n", timestamp, e.what());
#endif
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fTotalAmps = %3.2f\n", timestamp, m_globalStatus.fTotalAmps);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fAmpsQuad = %3.2f\n", timestamp, m_globalStatus.fAmpsQuad);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fAmpsDewA = %3.2f\n", timestamp, m_globalStatus.fAmpsDewA);
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::getPowerMetric] fAmpsDewB = %3.2f\n", timestamp, m_globalStatus.fAmpsDewB);
    fflush(Logfile);
#endif
    
    return nErr;
}


#pragma mark command and response functions

int CPegasusPPBAPower::ppbCommand(const char *pszszCmd, char *pszResult, unsigned long nResultMaxLen)
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
	fprintf(Logfile, "[%s] CPegasusPPBAPower::ppbCommand Sending %s\n", timestamp, pszszCmd);
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
        fprintf(Logfile, "[%s] CPegasusPPBAPower::ppbCommand response \"%s\"\n", timestamp, szResp);
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
		fprintf(Logfile, "[%s] CPegasusPPBAPower::ppbCommand response copied to pszResult : \"%s\"\n", timestamp, pszResult);
		fflush(Logfile);
#endif
    }
    return nErr;
}

int CPegasusPPBAPower::readResponse(char *pszRespBuffer, unsigned long nBufferLen)
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
			fprintf(Logfile, "[%s] CPegasusPPBAPower::readResponse timeout\n", timestamp);
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


int CPegasusPPBAPower::parseResp(char *pszResp, std::vector<std::string>  &svParsedResp)
{
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBAPower::parseResp parsing \"%s\"\n", timestamp, pszResp);
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
        fprintf(Logfile, "[%s] CPegasusPPBAPower::parseResp sSegment : %s\n", timestamp, sSegment.c_str());
        fflush(Logfile);
#endif
    }

    svParsedResp = svSeglist;


    return PLUGIN_OK;
}


void CPegasusPPBAPower::log(std::string sLog)
{
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBAPower::log] %s\n", timestamp, sLog.c_str());
    fflush(Logfile);
#endif

}
