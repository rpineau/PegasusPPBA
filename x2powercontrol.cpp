#include "x2powercontrol.h"

X2PowerControl::X2PowerControl(const char* pszDisplayName,
										const int& nInstanceIndex,
										SerXInterface						* pSerXIn,
										TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
										SleeperInterface					* pSleeperIn,
										BasicIniUtilInterface				* pIniUtilIn,
										LoggerInterface						* pLoggerIn,
										MutexInterface						* pIOMutexIn,
										TickCountInterface					* pTickCountIn):m_bLinked(0)
{
    char portName[255];
	std::string sLabel;
    int i;

	m_pTheSkyXForMounts = pTheSkyXIn;
	m_pSleeper = pSleeperIn;
	m_pIniUtil = pIniUtilIn;
	m_pTickCount = pTickCountIn;
    m_pLogger = pLoggerIn;

	m_nISIndex = nInstanceIndex;

    m_pIOMutex = pIOMutexIn;
    m_pSavedMutex = pIOMutexIn;

    m_pSavedSerX = pSerXIn;
    m_PowerPorts.SetSerxPointer(pSerXIn);
    
    if (m_pIniUtil) {
		// load port names
		for(i=0; i<NB_PORTS; i++) {
            switch(i) {
                case 0 :
                    sLabel = "4x12V";
                    break;

                case 1 :
                    sLabel = "Adjustable output";
                    break;

                case 2 :
                    sLabel = "Dew Heater A";
                    break;

                case 3:
                    sLabel = "Dew Heater B";
                    break;
            }
            m_pIniUtil->readString(PARENT_KEY, m_IniPortKey[i].c_str(), sLabel.c_str(), portName, 255);
            m_sPortNames.push_back(std::string(portName));
		}
    }
}

X2PowerControl::~X2PowerControl()
{
	//Delete objects used through composition
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();

    if (m_pSavedSerX)
        delete m_pSavedSerX;
    if (m_pSavedMutex)
        delete m_pSavedMutex;

}

int X2PowerControl::establishLink(void)
{
    int nErr = SB_OK;
    char szPort[DRIVER_MAX_STRING];
    
    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    nErr = m_PowerPorts.Connect(szPort);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;
    
    return nErr;
}

int X2PowerControl::terminateLink(void)
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        m_PowerPorts.Disconnect(m_nInstanceCount);
    }
    m_bLinked = false;

    // We're not connected, so revert to our saved interfaces
    m_PowerPorts.SetSerxPointer(m_pSavedSerX);
    m_pIOMutex = m_pSavedMutex;

    m_bLinked = false;

    return SB_OK;
}

bool X2PowerControl::isLinked() const
{
	return m_bLinked;
}


void X2PowerControl::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "Pegasus Astro PPBA";
}
void X2PowerControl::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = "Pegasus Astro PPBA";
}
void X2PowerControl::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = "Pegasus Astro PPBA power port controll";
}
void X2PowerControl::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(!m_bLinked) {
        str="NA";
    }
    else {
        // get firmware version
        std::string sFirmware;
        m_PowerPorts.getFirmwareVersionString(sFirmware);
        str = sFirmware.c_str();
    }
}

void X2PowerControl::deviceInfoModel(BasicStringInterface& str)
{
	str = "Pegasus Astro PPBA";
}

void X2PowerControl::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "Pegasus Astro PPBA X2 plugin by Rodolphe Pineau";
}

double X2PowerControl::driverInfoVersion(void) const
{
	return DRIVER_VERSION;
}

int X2PowerControl::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;


    if (!strcmp(pszName, LinkInterface_Name))
        *ppVal = (LinkInterface*)this;

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    else if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
        *ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);

    else if (!strcmp(pszName, CircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<CircuitLabelsInterface*>(this);

    else if (!strcmp(pszName, SetCircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<SetCircuitLabelsInterface*>(this);

    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
        *ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);

	return 0;
}

#pragma mark - UI binding

int X2PowerControl::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    bool bOn;
    int nTmp;
    float fTmp;
    char szTmp[256];
    float fAverageAmps;
    float fAmpHours;
    float fWattHours;
    unsigned long nUptimeMs;
    float fTotalAmps;
    float fAmpsQuad;
    float fAmpsDewA;
    float fAmpsDewB;
    std::vector<int> bootStates;

    if (NULL == ui)
        return ERR_POINTER;

    nErr = ui->loadUserInterface("PegasusPPBA.ui", deviceType(), m_nISIndex);
    if (nErr)
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    if(m_bLinked) {
        m_PowerPorts.getConsolidatedStatus();

        fTmp = m_PowerPorts.getVoltage();
        snprintf(szTmp, 256, "%3.2f V", fTmp);
        dx->setText("voltage", szTmp);
        
        fTmp = m_PowerPorts.getTemp();
        snprintf(szTmp, 256, "%3.2f ºC", fTmp);
        dx->setText("temperature", szTmp);

        nTmp = m_PowerPorts.getHumidity();
        snprintf(szTmp, 256, "%d%%", nTmp);
        dx->setText("humidity", szTmp);

        fTmp = m_PowerPorts.getDewPoint();
        snprintf(szTmp, 256, "%3.2f ºC", fTmp);
        dx->setText("dewPoint", szTmp);

        m_PowerPorts.getPower(fAverageAmps, fAmpHours, fWattHours, nUptimeMs);
        
        snprintf(szTmp, 256, "%3.2f A", fAverageAmps);
        dx->setText("currentDraw", szTmp);

        snprintf(szTmp, 256, "%3.2f Ah", fAmpHours);
        dx->setText("ampHours", szTmp);

        snprintf(szTmp, 256, "%3.2f Wh", fWattHours);
        dx->setText("wattHours", szTmp);

        m_PowerPorts.getPowerMetric(fTotalAmps, fAmpsQuad, fAmpsDewA, fAmpsDewB);
        
        snprintf(szTmp, 256, "%3.2f A", fTotalAmps);
        dx->setText("totalCurrentDraw", szTmp);

        snprintf(szTmp, 256, "%3.2f A", fAmpsQuad);
        dx->setText("currentDraw12V", szTmp);

        snprintf(szTmp, 256, "%3.2f A", fAmpsDewA);
        dx->setText("currentDrawDewA", szTmp);

        snprintf(szTmp, 256, "%3.2f A", fAmpsDewB);
        dx->setText("currentDrawDewB", szTmp);

        if(m_PowerPorts.isAutoDewOn()) {
            dx->setChecked("checkBox_9", true);
            dx->setEnabled("pushButton_3",false);
            dx->setEnabled("pushButton_4",false);
            dx->setEnabled("dewHeaterA",false);
            dx->setEnabled("dewHeaterB",false);
            dx->setEnabled("horizontalSlider", true);
        }
        else {
            dx->setChecked("checkBox_9", false);
            dx->setEnabled("pushButton_3",true);
            dx->setEnabled("pushButton_4",true);
            dx->setEnabled("dewHeaterA",true);
            dx->setEnabled("dewHeaterB",true);
            dx->setEnabled("spinBox", false);
        }
        nTmp = m_PowerPorts.getDewHeaterPWM(PWMA);
        dx->setPropertyInt("dewHeaterA", "value", nTmp);
        nTmp = m_PowerPorts.getDewHeaterPWM(PWMB);
        dx->setPropertyInt("dewHeaterB", "value", nTmp);

        m_PowerPorts.getAutoDewAggressivness(nTmp);
        dx->setPropertyInt("spinBox", "value", nTmp);

        nTmp = m_PowerPorts.getAdjVoltage();
        switch(nTmp) {
            case 3:
                dx->setCurrentIndex("comboBox", 0);
                break;
            case 5:
                dx->setCurrentIndex("comboBox", 1);
                break;
            case 8:
                dx->setCurrentIndex("comboBox", 2);
                break;
            case 9:
                dx->setCurrentIndex("comboBox", 3);
                break;
            case 12:
                dx->setCurrentIndex("comboBox", 4);
                break;
            default:
                dx->setCurrentIndex("comboBox", 0);
                break;
        }

        bOn = m_PowerPorts.getOnBootPortOn(QUAD12);
        dx->setChecked("checkBox_5", bOn?1:0);
        bOn = m_PowerPorts.getOnBootPortOn(ADJ);
        dx->setChecked("checkBox_6", bOn?1:0);
        
        m_PowerPorts.getLedStatus(nTmp);
        switch(nTmp){
            case ON:
                dx->setChecked("radioButton_3", 1);
                break;
            case OFF:
                dx->setChecked("radioButton_4", 1);
                break;
        }
    }
	else {
		dx->setEnabled("pushButton",false);
		dx->setEnabled("pushButton_2",false);

		dx->setEnabled("groupBox_3",false);
		dx->setEnabled("groupBox_6",false);
		dx->setEnabled("groupBox_7",false);
		dx->setEnabled("groupBox_2",false);

		dx->setEnabled("radioButton",false);
		dx->setEnabled("radioButton_2",false);
	}

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        m_PowerPorts.setOnBootPortOn(QUAD12, dx->isChecked("checkBox_5")==1?true:false);
        m_PowerPorts.setOnBootPortOn(ADJ, dx->isChecked("checkBox_6")==1?true:false);
    }
    return nErr;

}

void X2PowerControl::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nPwmPort;
    bool bAutoDewEnabled = false;
    int nErr = SB_OK;
    int nTmp;
    float fTmp;
    char szTmp[256];
    float fAverageAmps;
    float fAmpHours;
    float fWattHours;
    unsigned long nUptimeMs;
    float fTotalAmps;
    float fAmpsQuad;
    float fAmpsDewA;
    float fAmpsDewB;

    if (!strcmp(pszEvent, "on_timer")) {
        if(m_bLinked) {
            m_PowerPorts.getConsolidatedStatus();

            fTmp = m_PowerPorts.getVoltage();
            snprintf(szTmp, 256, "%3.2f V", fTmp);
            uiex->setText("voltage", szTmp);
            
            fTmp = m_PowerPorts.getTemp();
            snprintf(szTmp, 256, "%3.2f ºC", fTmp);
            uiex->setText("temperature", szTmp);
            
            nTmp = m_PowerPorts.getHumidity();
            snprintf(szTmp, 256, "%d%%", nTmp);
            uiex->setText("humidity", szTmp);
            
            fTmp = m_PowerPorts.getDewPoint();
            snprintf(szTmp, 256, "%3.2f ºC", fTmp);
            uiex->setText("dewPoint", szTmp);
            
            m_PowerPorts.getPower(fAverageAmps, fAmpHours, fWattHours, nUptimeMs);
            snprintf(szTmp, 256, "%3.2f A", fAverageAmps);
            uiex->setText("currentDraw", szTmp);
            
            snprintf(szTmp, 256, "%3.2f Ah", fAmpHours);
            uiex->setText("ampHours", szTmp);
            
            snprintf(szTmp, 256, "%3.2f Wh", fWattHours);
            uiex->setText("wattHours", szTmp);
            
            m_PowerPorts.getPowerMetric(fTotalAmps, fAmpsQuad, fAmpsDewA, fAmpsDewB);

            snprintf(szTmp, 256, "%3.2f A", fTotalAmps);
            uiex->setText("totalCurrentDraw", szTmp);
            
            snprintf(szTmp, 256, "%3.2f A", fAmpsQuad);
            uiex->setText("currentDraw12V", szTmp);
            
            snprintf(szTmp, 256, "%3.2f A", fAmpsDewA);
            uiex->setText("currentDrawDewA", szTmp);
            
            snprintf(szTmp, 256, "%3.2f A", fAmpsDewB);
            uiex->setText("currentDrawDewB", szTmp);
        }
    }
    else if (!strcmp(pszEvent, "on_checkBox_9_stateChanged")) {
        bAutoDewEnabled = (uiex->isChecked("checkBox_9")?true:false);
        nErr = m_PowerPorts.setAutoDewOn(bAutoDewEnabled);
        uiex->setEnabled("spinBox", bAutoDewEnabled);
        // read PWM value now that autodew is disabled
        if(!bAutoDewEnabled) {
            nTmp = m_PowerPorts.getDewHeaterPWM(PWMA);
            uiex->setPropertyInt("dewHeaterA", "value", nTmp);
            nTmp = m_PowerPorts.getDewHeaterPWM(PWMB);
            uiex->setPropertyInt("dewHeaterB", "value", nTmp);
            uiex->setEnabled("dewHeaterA",true);
            uiex->setEnabled("dewHeaterB",true);
            uiex->setEnabled("pushButton_3",true);
            uiex->setEnabled("pushButton_4",true);
        }
        else {
            uiex->setEnabled("dewHeaterA",false);
            uiex->setEnabled("dewHeaterB",false);
            uiex->setEnabled("pushButton_3",false);
            uiex->setEnabled("pushButton_4",false);
        }
    }
    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        uiex->propertyInt("dewHeaterA", "value", nPwmPort);
        nErr = m_PowerPorts.setDewHeaterPWMVal(PWMA, nPwmPort);
    }

    else if (!strcmp(pszEvent, "on_pushButton_4_clicked")) {
        uiex->propertyInt("dewHeaterB", "value", nPwmPort);
        nErr = m_PowerPorts.setDewHeaterPWMVal(PWMB, nPwmPort);
    }

    else if (!strcmp(pszEvent, "on_comboBox_currentIndexChanged")) {
        nTmp = uiex->currentIndex("comboBox");
        switch(nTmp) {
            case 0:
                m_PowerPorts.setAdjVoltage(3);
                break;
            case 1:
                m_PowerPorts.setAdjVoltage(5);
                break;
            case 2:
                m_PowerPorts.setAdjVoltage(8);
                break;
            case 3:
                m_PowerPorts.setAdjVoltage(9);
                break;
            case 4:
                m_PowerPorts.setAdjVoltage(12);
                break;
            default:
                m_PowerPorts.setAdjVoltage(3);
                break;
        }
    }
    else if (!strcmp(pszEvent, "on_radioButton_3_clicked")) {
        m_PowerPorts.setLedStatus((uiex->isChecked("radioButton_3")?ON:OFF));
    }
    else if (!strcmp(pszEvent, "on_radioButton_4_clicked")) {
        m_PowerPorts.setLedStatus((uiex->isChecked("radioButton_4")?OFF:ON));
    }    
}

int X2PowerControl::numberOfCircuits(int& nNumber)
{
	nNumber = m_PowerPorts.getPortCount();
	return SB_OK;
}

int X2PowerControl::circuitState(const int& nIndex, bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    if (nIndex >= 0 && nIndex<m_PowerPorts.getPortCount())
        bZeroForOffOneForOn = m_PowerPorts.getPortOn(nIndex+1);
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;

    return nErr;
}

int X2PowerControl::setCircuitState(const int& nIndex, const bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());
	if (nIndex >= 0 && nIndex < m_PowerPorts.getPortCount())
        nErr = m_PowerPorts.setPortOn(nIndex+1, bZeroForOffOneForOn);
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;
	return nErr;
}

int X2PowerControl::circuitLabel(const int &nZeroBasedIndex, BasicStringInterface &str)
{
    int nErr = SB_OK;
    std::string sLabel;
    
    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        str = m_sPortNames[nZeroBasedIndex].c_str();
    }
    else {
        switch(nZeroBasedIndex) {
            case 0 :
                sLabel = "4x12V";
                break;

            case 1 :
                sLabel = "Adjustable output";
                break;

            case 2 :
                sLabel = "Dew Heater A";
                break;

            case 3:
                sLabel = "Dew Heater B";
                break;

			case 4:
				sLabel = "USB2 ports";
				break;
        }

        str = sLabel.c_str();
    }

    return nErr;
}

int X2PowerControl::setCircuitLabel(const int &nZeroBasedIndex, const char *str)
{
    int nErr = SB_OK;

    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        m_sPortNames[nZeroBasedIndex] = str;
        m_pIniUtil->writeString(PARENT_KEY, m_IniPortKey[nZeroBasedIndex].c_str(), str);
    }
    else {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}
//
// SerialPortParams2Interface
//
#pragma mark - SerialPortParams2Interface

void X2PowerControl::portName(BasicStringInterface& str) const
{
    char szPortName[SERIAL_BUFFER_SIZE];

    portNameOnToCharPtr(szPortName, SERIAL_BUFFER_SIZE);

    str = szPortName;

}

void X2PowerControl::setPortName(const char* szPort)
{
    if (m_pIniUtil) {
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORTNAME, szPort);
    }

}


void X2PowerControl::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize,DEF_PORT_NAME);

    if (m_pIniUtil) {
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
    }

}

#pragma mark - MultiConnectionDeviceInterface

int X2PowerControl::deviceIdentifier(BasicStringInterface &sIdentifier)
{
    sIdentifier = "PPBA_EXT";
    return SB_OK;
}

int X2PowerControl::isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible)
{
    for (int nIndex = 0; nIndex < nPeerArraySize; ++nIndex)
    {
        X2FocuserExt *pPeer = dynamic_cast<X2FocuserExt*>(ppPeerArray[nIndex]);
        if (pPeer == NULL)
        {
            bConnectionPossible = false;
            return ERR_POINTER;
        }
    }

    bConnectionPossible = true;
    return SB_OK;
}

int X2PowerControl::useResource(MultiConnectionDeviceInterface *pPeer)
{

    X2FocuserExt *pFocuserPeer = dynamic_cast<X2FocuserExt*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; // Peer must be a focuser pointer
    }

    // Use the resources held by the specified peer
    m_pIOMutex = pFocuserPeer->m_pSavedMutex;
    m_PowerPorts.SetSerxPointer(pFocuserPeer->m_pSavedSerX);
    return SB_OK;

}

int X2PowerControl::swapResource(MultiConnectionDeviceInterface *pPeer)
{

    X2FocuserExt *pFocuserPeer = dynamic_cast<X2FocuserExt*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; //  Peer must be a focuser pointer
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
