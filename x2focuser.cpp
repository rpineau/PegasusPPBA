#include "x2focuser.h"


X2FocuserExt::X2FocuserExt(const char* pszDisplayName,
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn)

{
	m_pTheSkyXForMounts = pTheSkyXIn;
	m_pSleeper          = pSleeperIn;
	m_pIniUtil          = pIniUtilIn;
	m_pLogger           = pLoggerIn;
	m_pTickCount        = pTickCountIn;

    m_pIOMutex          = pIOMutexIn;
    m_pSavedMutex       = pIOMutexIn;

    m_pSavedSerX        = pSerXIn;
    m_PegasusPPBAExtFoc.SetSerxPointer(pSerXIn);

    m_PegasusPPBAExtFoc.setLogger(m_pLogger);
    m_PegasusPPBAExtFoc.setSleeper(m_pSleeper);

	m_bLinked = false;
	m_nPosition = 0;
    m_fLastTemp = -273.15f; // aboslute zero :)
    m_bReverseEnabled = false;

    // Read in settings
    if (m_pIniUtil) {
        m_PegasusPPBAExtFoc.setPosLimit(m_pIniUtil->readInt(PARENT_KEY_FOC, POS_LIMIT, 0));
        m_PegasusPPBAExtFoc.enablePosLimit(m_pIniUtil->readInt(PARENT_KEY_FOC, POS_LIMIT_ENABLED, 0) == 0 ? false : true);
        m_bReverseEnabled = m_pIniUtil->readInt(PARENT_KEY_FOC, REVERSE_ENABLED, 0) == 0 ? false : true;
    }


}

X2FocuserExt::~X2FocuserExt()
{

	//Delete objects used through composition
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
    if (m_pSavedSerX)
        delete m_pSavedSerX;
    if (m_pSavedMutex)
        delete m_pSavedMutex;
}

#pragma mark - DriverRootInterface

int	X2FocuserExt::queryAbstraction(const char* pszName, void** ppVal)
{
    *ppVal = NULL;

    if (!strcmp(pszName, LinkInterface_Name))
        *ppVal = (LinkInterface*)this;

    else if (!strcmp(pszName, FocuserGotoInterface2_Name))
        *ppVal = (FocuserGotoInterface2*)this;

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    else if (!strcmp(pszName, FocuserTemperatureInterface_Name))
        *ppVal = dynamic_cast<FocuserTemperatureInterface*>(this);

    else if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
        *ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);

    return SB_OK;
}

#pragma mark - DriverInfoInterface
void X2FocuserExt::driverInfoDetailedInfo(BasicStringInterface& str) const
{
        str = "Pegasus Astro PPBA_EXT X2 plugin by Rodolphe Pineau";
}

double X2FocuserExt::driverInfoVersion(void) const
{
	return DRIVER_VERSION_FOC;
}

void X2FocuserExt::deviceInfoNameShort(BasicStringInterface& str) const
{
    X2FocuserExt* pMe = (X2FocuserExt*)this;

    if(!m_bLinked) {
        str="NA";
    }
    else {
        pMe->deviceInfoModel(str);
    }
}

void X2FocuserExt::deviceInfoNameLong(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2FocuserExt::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2FocuserExt::deviceInfoFirmwareVersion(BasicStringInterface& str)
{

    if(!m_bLinked) {
        str="NA";
    }
    else {
    // get firmware version
        std::string sFirmware;
        m_PegasusPPBAExtFoc.getFirmwareString(sFirmware);
        str = sFirmware.c_str();
    }
}

void X2FocuserExt::deviceInfoModel(BasicStringInterface& str)
{

    if(!m_bLinked) {
        str="NA";
    }
    else {
        // get model versiona
        std::string sDeviceType;
        m_PegasusPPBAExtFoc.getDeviceTypeString(sDeviceType);
        str = sDeviceType.c_str();
    }
}

#pragma mark - LinkInterface
int	X2FocuserExt::establishLink(void)
{
    char szPort[DRIVER_MAX_STRING];
    int err;

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    err = m_PegasusPPBAExtFoc.Connect(szPort);
    if(err)
        m_bLinked = false;
    else
        m_bLinked = true;

    if(m_bLinked)
        err = m_PegasusPPBAExtFoc.setReverseEnable(m_bReverseEnabled);

    return err;
}

int	X2FocuserExt::terminateLink(void)
{
    if(!m_bLinked)
        return SB_OK;

    X2MutexLocker ml(GetMutex());
    // m_PegasusPPBAExtFoc.Disconnect(m_nInstanceCount);
    m_PegasusPPBAExtFoc.Disconnect(1);
    m_bLinked = false;
    // We're not connected, so revert to our saved interfaces
    m_PegasusPPBAExtFoc.SetSerxPointer(m_pSavedSerX);
    m_pIOMutex = m_pSavedMutex;

    return SB_OK;
}

bool X2FocuserExt::isLinked(void) const
{
	return m_bLinked;
}

#pragma mark - ModalSettingsDialogInterface
int	X2FocuserExt::initModalSettingsDialog(void)
{
    return SB_OK;
}

int	X2FocuserExt::execModalSettingsDialog(void)
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*					ui = uiutil.X2UI();
    X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    int nMaxSpeed = 0;
    int nPosition = 0;
    bool bLimitEnabled = false;
    int nPosLimit = 0;
    int nBacklashSteps = 0;
    bool bBacklashEnabled = false;
    bool bReverse = false;
    int nDeviceType = PPBA_EXT_FOC;
    int nStepping;

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("PegasusPPBAExtFocuser.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());
    
	// set controls values
    if(m_bLinked) {
        // get data from device
        m_PegasusPPBAExtFoc.getDeviceType(nDeviceType);
        
        // enable all controls
        // motor max speed
        nErr = m_PegasusPPBAExtFoc.getMotoMaxSpeed(nMaxSpeed);
        //if(nErr)
        //    return nErr;
        dx->setEnabled("maxSpeed", true);
        dx->setEnabled("pushButton", true);
        dx->setPropertyInt("maxSpeed", "value", nMaxSpeed);

        // new position (set to current )
        nErr = m_PegasusPPBAExtFoc.getPosition(nPosition);
        //if(nErr)
        //   return nErr;
        dx->setEnabled("newPos", true);
        dx->setEnabled("pushButton_2", true);
        dx->setPropertyInt("newPos", "value", nPosition);

        // microsteping
        nErr = m_PegasusPPBAExtFoc.getMicrostepping(nStepping);
        dx->setEnabled("comboBox", true);
        dx->setEnabled("pushButton_3", true);
        dx->setCurrentIndex("comboBox", nStepping-1);
        // reverse
        dx->setEnabled("reverseDir", true);
        nErr = m_PegasusPPBAExtFoc.getReverseEnable(bReverse);
        if(bReverse)
            dx->setChecked("reverseDir", true);
        else
            dx->setChecked("reverseDir", false);

        // backlash
        dx->setEnabled("backlashSteps", true);
        nErr = m_PegasusPPBAExtFoc.getBacklashComp(nBacklashSteps);
        //if(nErr)
        //    return nErr;
        dx->setPropertyInt("backlashSteps", "value", nBacklashSteps);

        if(!nBacklashSteps)  // backlash = 0 means disabled.
            bBacklashEnabled = false;
        else
            bBacklashEnabled = true;
        if(bBacklashEnabled)
            dx->setChecked("backlashEnable", true);
        else
            dx->setChecked("backlashEnable", false);
    }
    else {
        // disable controls when not connected
        dx->setEnabled("maxSpeed", false);
        dx->setPropertyInt("maxSpeed", "value", 0);
        dx->setEnabled("pushButton", false);
        dx->setEnabled("newPos", false);
        dx->setPropertyInt("newPos", "value", 0);
        dx->setEnabled("reverseDir", false);
        dx->setEnabled("pushButton_2", false);
        dx->setEnabled("comboBox", false);
        dx->setEnabled("pushButton_3", false);

        dx->setEnabled("backlashSteps", false);
        dx->setPropertyInt("backlashSteps", "value", 0);
        dx->setEnabled("backlashEnable", false);

        dx->setEnabled("radioButton", false);
        dx->setEnabled("radioButton_2", false);
    }

    // linit is done in software so it's always enabled.
    dx->setEnabled("posLimit", true);
    dx->setEnabled("limitEnable", true);
    dx->setPropertyInt("posLimit", "value", m_PegasusPPBAExtFoc.getPosLimit());
    if(m_PegasusPPBAExtFoc.isPosLimitEnabled())
        dx->setChecked("limitEnable", true);
    else
        dx->setChecked("limitEnable", false);

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retrieve values from the user interface
    if (bPressedOK) {
        nErr = SB_OK;
        // get limit option
        bLimitEnabled = dx->isChecked("limitEnable") == 0 ? false : true;
        dx->propertyInt("posLimit", "value", nPosLimit);
        if(bLimitEnabled && nPosLimit>0) { // a position limit of 0 doesn't make sense :)
            m_PegasusPPBAExtFoc.setPosLimit(nPosLimit);
            m_PegasusPPBAExtFoc.enablePosLimit(bLimitEnabled);
        } else {
            m_PegasusPPBAExtFoc.setPosLimit(nPosLimit);
            m_PegasusPPBAExtFoc.enablePosLimit(false);
        }
		if(m_bLinked) {
			// get reverse
            bReverse = dx->isChecked("reverseDir") == 0 ? false : true;
			nErr = m_PegasusPPBAExtFoc.setReverseEnable(bReverse);
			if(nErr)
				return nErr;
            // save value to config
            nErr = m_pIniUtil->writeInt(PARENT_KEY_FOC, REVERSE_ENABLED, bReverse);
            if(nErr)
                return nErr;

			// get backlash if enable, disbale if needed
            bBacklashEnabled = dx->isChecked("backlashEnable") == 0 ? false : true;
			if(bBacklashEnabled) {
				dx->propertyInt("backlashSteps", "value", nBacklashSteps);
				nErr = m_PegasusPPBAExtFoc.setBacklashComp(nBacklashSteps);
				if(nErr)
					return nErr;
			}
			else {
				nErr = m_PegasusPPBAExtFoc.setBacklashComp(0); // disable backlash comp
				if(nErr)
					return nErr;
			}
		}
        // save values to config
        nErr |= m_pIniUtil->writeInt(PARENT_KEY_FOC, POS_LIMIT, nPosLimit);
        nErr |= m_pIniUtil->writeInt(PARENT_KEY_FOC, POS_LIMIT_ENABLED, bLimitEnabled?1:0);
    }
    return nErr;
}

void X2FocuserExt::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = SB_OK;
    int nTmpVal;
    char szErrorMessage[TEXT_BUFFER_SIZE];
    
    if(!m_bLinked)
        return;

    if (!strcmp(pszEvent, "on_pushButton_clicked")) {
        uiex->propertyInt("maxSpeed", "value", nTmpVal);
        nErr = m_PegasusPPBAExtFoc.setMotoMaxSpeed(nTmpVal);
        if(nErr) {
            snprintf(szErrorMessage, TEXT_BUFFER_SIZE, "Error setting max speed : Error %d", nErr);
            uiex->messageBox("Set max Speed", szErrorMessage);
            return;
        }
    }
    // new position
    else if (!strcmp(pszEvent, "on_pushButton_2_clicked")) {
        uiex->propertyInt("newPos", "value", nTmpVal);
        nErr = m_PegasusPPBAExtFoc.syncMotorPosition(nTmpVal);
        if(nErr) {
            snprintf(szErrorMessage, TEXT_BUFFER_SIZE, "Error setting new position : Error %d", nErr);
            uiex->messageBox("Set new Position", szErrorMessage);
            return;
        }
    }
    // change microstepping
    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        nTmpVal = uiex->currentIndex("comboBox");
        nErr = m_PegasusPPBAExtFoc.setMicrostepping(nTmpVal+1);
        if(nErr) {
            snprintf(szErrorMessage, TEXT_BUFFER_SIZE, "Error setting new microstepping : Error %d", nErr);
            uiex->messageBox("Set new microstepping", szErrorMessage);
            return;
        }
    }
}

#pragma mark - FocuserGotoInterface2
int	X2FocuserExt::focPosition(int& nPosition)
{
    int err;

    if(!m_bLinked)
        return NOT_CONNECTED_PPBA_EXT_FOC;

    X2MutexLocker ml(GetMutex());

    err = m_PegasusPPBAExtFoc.getPosition(nPosition);
    m_nPosition = nPosition;
    return err;
}

int	X2FocuserExt::focMinimumLimit(int& nMinLimit)
{
	nMinLimit = 0;
    return SB_OK;
}

int	X2FocuserExt::focMaximumLimit(int& nPosLimit)
{
    if(m_PegasusPPBAExtFoc.isPosLimitEnabled()) {
        nPosLimit = m_PegasusPPBAExtFoc.getPosLimit();
    }
    else
        nPosLimit = 100000;

    return SB_OK;
}

int	X2FocuserExt::focAbort()
{   int err;

    if(!m_bLinked)
        return NOT_CONNECTED_PPBA_EXT_FOC;

    X2MutexLocker ml(GetMutex());
    err = m_PegasusPPBAExtFoc.haltFocuser();
    return err;
}

int	X2FocuserExt::startFocGoto(const int& nRelativeOffset)
{
    if(!m_bLinked)
        return NOT_CONNECTED_PPBA_EXT_FOC;

    X2MutexLocker ml(GetMutex());
    m_PegasusPPBAExtFoc.moveRelativeToPosision(nRelativeOffset);
    return SB_OK;
}

int	X2FocuserExt::isCompleteFocGoto(bool& bComplete) const
{
    int err;

    if(!m_bLinked)
        return NOT_CONNECTED_PPBA_EXT_FOC;

    X2FocuserExt* pMe = (X2FocuserExt*)this;
    X2MutexLocker ml(pMe->GetMutex());

    err = pMe->m_PegasusPPBAExtFoc.isGoToComplete(bComplete);

    return err;
}

int	X2FocuserExt::endFocGoto(void)
{
    int err;
    if(!m_bLinked)
        return NOT_CONNECTED_PPBA_EXT_FOC;

    X2MutexLocker ml(GetMutex());
    err = m_PegasusPPBAExtFoc.getPosition(m_nPosition);
    return err;
}

int X2FocuserExt::amountCountFocGoto(void) const
{ 
	return 3;
}

int	X2FocuserExt::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)
{
	switch (nZeroBasedIndex)
	{
		default:
		case 0: strDisplayName="10 steps"; nAmount=10;break;
		case 1: strDisplayName="100 steps"; nAmount=100;break;
		case 2: strDisplayName="1000 steps"; nAmount=1000;break;
	}
	return SB_OK;
}

int	X2FocuserExt::amountIndexFocGoto(void)
{
	return 0;
}

#pragma mark - FocuserTemperatureInterface
int X2FocuserExt::focTemperature(double &dTemperature)
{
    int err = SB_OK;

    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        dTemperature = -100.0f;
        return NOT_CONNECTED_PPBA_EXT_FOC;
    }

    // Taken from Richard's Robofocus plugin code.
    // this prevent us from asking the temperature too often
    static CStopWatch timer;

    if(timer.GetElapsedSeconds() > 30.0f || m_fLastTemp < -99.0f) {
        X2MutexLocker ml(GetMutex());
        err = m_PegasusPPBAExtFoc.getTemperature(m_fLastTemp);
        timer.Reset();
    }

    dTemperature = m_fLastTemp;

    return err;
}

#pragma mark - SerialPortParams2Interface

void X2FocuserExt::portName(BasicStringInterface& str) const
{
    char szPortName[DRIVER_MAX_STRING];

    portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

    str = szPortName;

}

void X2FocuserExt::setPortName(const char* pszPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY_FOC, CHILD_KEY_PORTNAME_FOC, pszPort);

}


void X2FocuserExt::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize, DEF_PORT_NAME_FOC);

    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY_FOC, CHILD_KEY_PORTNAME_FOC, pszPort, pszPort, nMaxSize);
    
}

int X2FocuserExt::deviceIdentifier(BasicStringInterface &sIdentifier)
{
    sIdentifier = "PPBA_EXT";
    return SB_OK;
}

int X2FocuserExt::isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible)
{
    for (int nIndex = 0; nIndex < nPeerArraySize; ++nIndex)
    {
        X2PowerControl *pPeer = dynamic_cast<X2PowerControl*>(ppPeerArray[nIndex]);
        if (pPeer == NULL)
        {
            bConnectionPossible = false;
            return ERR_POINTER;
        }
    }

    bConnectionPossible = true;
    return SB_OK;

}

int X2FocuserExt::useResource(MultiConnectionDeviceInterface *pPeer)
{
    X2PowerControl *pFocuserPeer = dynamic_cast<X2PowerControl*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; // Peer must be a power control  pointer
    }

    // Use the resources held by the specified peer
    m_pIOMutex = pFocuserPeer->m_pSavedMutex;
    m_PegasusPPBAExtFoc.SetSerxPointer(pFocuserPeer->m_pSavedSerX);
    return SB_OK;

}

int X2FocuserExt::swapResource(MultiConnectionDeviceInterface *pPeer)
{

    X2PowerControl *pFocuserPeer = dynamic_cast<X2PowerControl*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; //  Peer must be a power control  pointer
    }

    // Swap this driver instance's resources for the ones held by pPeer
    MutexInterface* pTempMutex = m_pSavedMutex;
    SerXInterface*  pTempSerX = m_pSavedSerX;

    m_pSavedMutex = pFocuserPeer->m_pSavedMutex;
    m_pSavedSerX = pFocuserPeer->m_pSavedSerX;

    pFocuserPeer->m_pSavedMutex = pTempMutex;
    pFocuserPeer->m_pSavedSerX = pTempSerX;

    return SB_OK;
}

