//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1


#include "pegasus_ppbaExtFocuser.h"

CPegasusPPBA_EXTFocuser::CPegasusPPBA_EXTFocuser()
{
    nDeviceType = NONE_FOC;


    m_nTargetPos = 0;
    m_nPosLimit = 0;
    m_bPosLimitEnabled = false;
    m_bAbborted = false;
    
    m_pSerx = NULL;
    m_pLogger = NULL;
    m_pSleeper = NULL;

    m_sFirmwareVersion.clear();

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PegasusPPBA_EXTFocuserLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusPPBA_EXTFocuserLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusPPBA_EXTFocuserLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::CPegasusPPBA_EXTFocuser] version %3.2f build 2021_08_13_2000.\n", timestamp, DRIVER_VERSION_FOC);
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::CPegasusPPBA_EXTFocuser] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusPPBA_EXTFocuser::~CPegasusPPBA_EXTFocuser()
{
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusPPBA_EXTFocuser::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    int nDevice;
    int nSpeed;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif

    // 9600 8N1
    if (!m_pSerx->isConnected()) {
        // nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
        nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY);
        if(nErr == 0) {
            m_bIsConnected = true;
            // m_pSleeper->sleep(1500);
        }
        else
            m_bIsConnected = false;
    }
    else
        m_bIsConnected = true;

    if(!m_bIsConnected)
        return nErr;


#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::Connect getting device type\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != PPBA_EXT_FOC) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }

    nErr = getFirmwareVersion(m_sFirmwareVersion);

    getMotoMaxSpeed(nSpeed);
    if(nSpeed == 65535) // WAY to fast for any stepper :)
        setMotoMaxSpeed(1000);
    return nErr;
}

void CPegasusPPBA_EXTFocuser::Disconnect(int nInstanceCount)
{
    if(m_bIsConnected && m_pSerx && nInstanceCount==1)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark move commands
int CPegasusPPBA_EXTFocuser::haltFocuser()
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;

    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = pppaCommand("XS:6\n", sResp);
	m_bAbborted = true;
	
	return nErr;
}

int CPegasusPPBA_EXTFocuser::gotoPosition(int nPos)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    if (m_bPosLimitEnabled && nPos>m_nPosLimit)
        return ERR_LIMITSEXCEEDED;

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::gotoPosition moving to %d\n", timestamp, nPos);
    fflush(Logfile);
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:3#%d\n", nPos);
    nErr = pppaCommand(szCmd, sResp);
    m_nTargetPos = nPos;

    return nErr;
}

int CPegasusPPBA_EXTFocuser::moveRelativeToPosision(int nSteps)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusPPBA_EXTFocuser::moveRelativeToPosision moving by %d steps\n", timestamp, nSteps);
    fflush(Logfile);
#endif

    m_nTargetPos = m_nCurpos + nSteps;

    nErr = gotoPosition(m_nTargetPos);
    
    return nErr;
}

#pragma mark command complete functions

int CPegasusPPBA_EXTFocuser::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    bool bIsMoving;
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    bComplete = false;
    nErr = isMotorMoving(bIsMoving);
    
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::isGoToComplete] motor is moving ? : %s\n", timestamp, bIsMoving?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::isGoToComplete] current position  : %d\n", timestamp, m_nCurpos);
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::isGoToComplete] target position   : %d\n", timestamp, m_nTargetPos);
    fflush(Logfile);
#endif

    if(bIsMoving) {
        return nErr;
    }
    nErr = getPosition(m_nCurpos);

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::isGoToComplete] current position  : %d\n", timestamp, m_nCurpos);
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::isGoToComplete] target position   : %d\n", timestamp, m_nTargetPos);
    fflush(Logfile);
#endif

	if(m_bAbborted) {
		bComplete = true;
		m_nTargetPos = m_nCurpos;
		m_bAbborted = false;
	}
    else if(m_nCurpos == m_nTargetPos)
        bComplete = true;
    else
        bComplete = false;
    return nErr;
}

int CPegasusPPBA_EXTFocuser::isMotorMoving(bool &bMoving)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = pppaCommand("XS:1\n", sResp);
    if(nErr)
        return nErr;
    // need parsing
    nErr = parseResp(sResp, svParsedResp,'#');
    if(nErr)
        return nErr;

    if(svParsedResp.size()>1) {
        if(svParsedResp[1].at(0) == '1')
            bMoving = false;
        else
            bMoving = false;
    }
    else {
        bMoving = false;
        nErr = COMMAND_FAILED_PPBA_EXT_FOC;
    }

    return nErr;
}

#pragma mark getters and setters
int CPegasusPPBA_EXTFocuser::getStatus()
{
    int nErr;
    std::string sResp;

    bool bExtPresent = false;
    std::vector<std::string> svParsedResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    // OK_UPB or OK_PPB
    nErr = pppaCommand("P#\n", sResp);
    if(nErr)
        return nErr;

    if(sResp.find("_OK") !=-1) {
        if(sResp.find("PPBA")!=-1) {
            nDeviceType = PPBA_EXT_FOC;
        }
        else {
            nDeviceType = NONE_FOC;
            return ERR_DEVICENOTSUPPORTED;
        }
        nErr = PLUGIN_OK_PPBA_EXT_FOC;
    }
    else {
        nErr = COMMAND_FAILED_PPBA_EXT_FOC;
    }

    // check if XS focuser is present
    nErr = pppaCommand("XS\n", sResp);
    if(nErr)
        return nErr;
    parseResp(sResp, svParsedResp);

    if(svParsedResp.size()>1 && svParsedResp[1].find("200")!=-1) {
            bExtPresent = true;
    }

    if(!bExtPresent) {
        nDeviceType = NONE_FOC;
        nErr = ERR_DEVICENOTSUPPORTED;
    }
    else {
        nDeviceType = PPBA_EXT_FOC;
    }
    return nErr;
}


int CPegasusPPBA_EXTFocuser::getMotoMaxSpeed(int &nSpeed)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = pppaCommand("XS:7\n", sResp);
    if(nErr)
        return nErr;

    // parse results.
    parseResp(sResp, svParsedResp, '#');
    if(svParsedResp.size()<2) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getMotoMaxSpeed] XS:7 parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_PPBA_EXT_FOC;
    }

    nSpeed = std::stoi(svParsedResp[1]);
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getMotoMaxSpeed] nSpeed = %d\n", timestamp, nSpeed);
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusPPBA_EXTFocuser::setMotoMaxSpeed(int nSpeed)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:7#%d\n", nSpeed);
    nErr = pppaCommand(szCmd, sResp);

    return nErr;
}

int CPegasusPPBA_EXTFocuser::getBacklashComp(int &nSteps)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    //
    // Backlash
    //
    nErr = pppaCommand("XS:10\n", sResp);
    if(nErr)
        return nErr;
    // parse results.
    parseResp(sResp, svParsedResp, '#');
    if(svParsedResp.size()<2) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getBacklashComp] XS:10 parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_PPBA_EXT_FOC;
    }
    nSteps = std::stoi(svParsedResp[1]);

    return nErr;
}

int CPegasusPPBA_EXTFocuser::setBacklashComp(int nSteps)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::setBacklashComp] setting backlash comp\n", timestamp);
    fflush(Logfile);
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:10#%d\n", nSteps);
    nErr = pppaCommand(szCmd, sResp);

    return nErr;
}


int CPegasusPPBA_EXTFocuser::getFirmwareVersion(std::string &sFirmwareVersion)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = pppaCommand("PV\n", sResp);
    if(nErr)
        return nErr;

    sFirmwareVersion.assign(sResp);

    return nErr;
}

void CPegasusPPBA_EXTFocuser::getFirmwareString(std::string &sFirmware)
{
    sFirmware.assign(m_sFirmwareVersion);

}

int CPegasusPPBA_EXTFocuser::getTemperature(double &dTemperature)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = pppaCommand("PA\n", sResp);
    if(nErr)
        return nErr;
    parseResp(sResp, svParsedResp);
    if(svParsedResp.size()<3) {
        dTemperature = -100.0f;
    }
    else {
        if(svParsedResp[3].find("nan")!=-1)
            dTemperature = -100.0f;
        else
            dTemperature = std::stof(svParsedResp[3]);
    }
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getTemperature] PA dTemperature = %3.2f\n", timestamp, dTemperature);
    fflush(Logfile);
#endif
    return nErr;
}

int CPegasusPPBA_EXTFocuser::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;
    //
    // Position
    //
    nErr = pppaCommand("XS:2\n", sResp);
    if(nErr)
        return nErr;
    // parse results.
    parseResp(sResp, svParsedResp, '#');
    if(svParsedResp.size()<2) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getPosition] XS:2 parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_PPBA_EXT_FOC;
    }

    nPosition = std::stoi(svParsedResp[1]);
    m_nCurpos = nPosition;
    
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getPosition] XS:2 position = %d\n", timestamp, nPosition);
    fflush(Logfile);
#endif
    return nErr;
}

int CPegasusPPBA_EXTFocuser::syncMotorPosition(int nPos)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:5#%d\n", nPos);
    nErr = pppaCommand(szCmd, sResp);
    return nErr;
}

int CPegasusPPBA_EXTFocuser::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus();
    nDevice = nDeviceType;
    
    return nErr;
}

void CPegasusPPBA_EXTFocuser::getDeviceTypeString(std::string &sDeviceType)
{
    if( nDeviceType== PPBA_EXT_FOC)
        sDeviceType = "eXternal focusser controller on PPBA";
    else
        sDeviceType = "Unknown device";

}

int CPegasusPPBA_EXTFocuser::getPosLimit()
{
    return m_nPosLimit;
}

void CPegasusPPBA_EXTFocuser::setPosLimit(int nLimit)
{
    m_nPosLimit = nLimit;
}

bool CPegasusPPBA_EXTFocuser::isPosLimitEnabled()
{
    return m_bPosLimitEnabled;
}

void CPegasusPPBA_EXTFocuser::enablePosLimit(bool bEnable)
{
    m_bPosLimitEnabled = bEnable;
}


int CPegasusPPBA_EXTFocuser::setReverseEnable(bool bEnabled)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(bEnabled)
        snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:8#%d\n", REVERSE);
    else
        snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:8#%d\n", NORMAL);

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::setReverseEnable] setting reverse : %s\n", timestamp, szCmd);
    fflush(Logfile);
#endif

    nErr = pppaCommand(szCmd, sResp);

#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    if(nErr) {
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::setReverseEnable] **** ERROR **** setting reverse (\"%s\") : %d\n", timestamp, szCmd, nErr);
        fflush(Logfile);
    }
#endif

    return nErr;
}

int CPegasusPPBA_EXTFocuser::getReverseEnable(bool &bEnabled)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    // Direction
    //
    nErr = pppaCommand("XS:8\n", sResp);
    if(nErr)
        return nErr;
    // parse results.
    parseResp(sResp, svParsedResp, '#');
    if(svParsedResp.size()<2) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getStepperStatus] XS:8 parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_PPBA_EXT_FOC;
    }
    bEnabled = std::stoi(svParsedResp[1])== 2?true:false;
    return nErr;
}



int CPegasusPPBA_EXTFocuser::setMicrostepping(int nStepping)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char szCmd[SERIAL_BUFFER_SIZE_FOC];
    std::string sResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE_FOC, "XS:9#%d\n", nStepping);

    nErr = pppaCommand(szCmd, sResp);
    if(nErr)
        return nErr;

    return nErr;
}

int CPegasusPPBA_EXTFocuser::getMicrostepping(int &nStepping)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sResp;
    std::vector<std::string>  svParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = pppaCommand("XS:9\n", sResp);
    if(nErr)
        return nErr;
    // parse results.
    parseResp(sResp, svParsedResp, '#');
    if(svParsedResp.size()<2) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::getMicrostepping] XS:9 parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_PPBA_EXT_FOC;
    }

    nStepping = std::stoi(svParsedResp[1]);
    return nErr;
}


#pragma mark command and response functions

int CPegasusPPBA_EXTFocuser::pppaCommand(const char *pszCmd, std::string &sResp, int nTimeout)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    unsigned long  ulBytesWrite;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::pppaCommand] Sending %s\n", timestamp, pszCmd);
    fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszCmd, strlen(pszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    // printf("Command %s sent. wrote %lu bytes\n", szCmd, ulBytesWrite);
    if(nErr){
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::pppaCommand] writeFile error %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

    // read response
    nErr = readResponse(sResp, nTimeout);
    if(nErr){
        return nErr;
    }
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::pppaCommand] response \"%s\"\n", timestamp, sResp.c_str());
    fflush(Logfile);
#endif

    return nErr;
}


int CPegasusPPBA_EXTFocuser::readResponse(std::string &sResp, int nTimeout)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    char pszBuf[SERIAL_BUFFER_SIZE_FOC];
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
    int nBytesWaiting = 0 ;
    int nbTimeouts = 0;
    sResp.clear();

    pszBufPtr = pszBuf;

    do {
        nErr = m_pSerx->bytesWaitingRx(nBytesWaiting);
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] nBytesWaiting = %d\n", timestamp, nBytesWaiting);
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] nBytesWaiting nErr = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        if(!nBytesWaiting) {
            if(nbTimeouts++ >= NB_RX_WAIT_FOC) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] bytesWaitingRx timeout, no data for %d loops\n", timestamp, NB_RX_WAIT_FOC);
                fflush(Logfile);
#endif
                nErr = ERR_RXTIMEOUT;
                break;
            }
            m_pSleeper->sleep(MAX_READ_WAIT_TIMEOUT_FOC);
            continue;
        }
        nbTimeouts = 0;
        if(ulTotalBytesRead + nBytesWaiting <= SERIAL_BUFFER_SIZE_FOC)
            nErr = m_pSerx->readFile(pszBufPtr, nBytesWaiting, ulBytesRead, nTimeout);
        else {
            nErr = ERR_RXTIMEOUT;
            break; // buffer is full.. there is a problem !!
        }
        if(nErr) {
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] readFile error.\n", timestamp);
            fflush(Logfile);
#endif
            return nErr;
        }

        if (ulBytesRead != nBytesWaiting) { // timeout
#if defined PLUGIN_DEBUG_PPBA_EXT_FOC && PLUGIN_DEBUG_PPBA_EXT_FOC >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] readFile Timeout Error\n", timestamp);
            fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] readFile nBytesWaiting = %d\n", timestamp, nBytesWaiting);
            fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::readResponse] readFile ulBytesRead = %lu\n", timestamp, ulBytesRead);
            fflush(Logfile);
#endif
        }

        ulTotalBytesRead += ulBytesRead;
        pszBufPtr+=ulBytesRead;
    } while (ulTotalBytesRead < SERIAL_BUFFER_SIZE_FOC  && *(pszBufPtr-1) != '\n');

    if(!ulTotalBytesRead)
        nErr = COMMAND_TIMEOUT_PPBA_EXT_FOC; // we didn't get an answer.. so timeout
    else
        *(pszBufPtr-1) = 0; //remove the \n

    sResp.assign(pszBuf);
    return nErr;
}


int CPegasusPPBA_EXTFocuser::parseResp(std::string sResp, std::vector<std::string> &svFields, char cSeparator)
{
    int nErr = PLUGIN_OK_PPBA_EXT_FOC;
    std::string sSegment;
    std::string sTmp;

    sResp = trim(sResp,"!#\r\n");
    if(!sResp.size()) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::parseFields] pszResp is empty\n", timestamp);
        fflush(Logfile);
#endif
        return COMMAND_FAILED_PPBA_EXT_FOC;
    }

    std::stringstream ssTmp(sResp);

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusPPBA_EXTFocuser::parseFields] no field found in '%s'\n", timestamp, sResp.c_str());
        fflush(Logfile);
#endif
        nErr = COMMAND_FAILED_PPBA_EXT_FOC;
    }
    return nErr;
}

std::string& CPegasusPPBA_EXTFocuser::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CPegasusPPBA_EXTFocuser::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CPegasusPPBA_EXTFocuser::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

