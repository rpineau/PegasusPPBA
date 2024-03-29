#ifndef __X2Power_H_
#define __X2Power_H_
#include <string.h>

#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/powercontroldriverinterface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/circuitlabels.h"
#include "../../licensedinterfaces/setcircuitlabels.h"
#include "../../licensedinterfaces/serialportparams2interface.h"
#include "../../licensedinterfaces/multiconnectiondeviceinterface.h"

#include "x2focuser.h"
#include "pegasus_PPBAPower.h"


// Forward declare the interfaces that this device is dependent upon
class X2FocuserExt;
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class SimpleIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;
class MultiConnectionDeviceInterface;


#define PARENT_KEY          "PA_PBBA"
#define CHILD_KEY_PORTNAME    "PortName"

#define CHILD_KEY_PWM_PORT6 "PWM_DUTY_6"
#define CHILD_KEY_PWM_PORT7 "PWM_DUTY_7"
#define CHILD_KEY_PORT1_NAME "PORT1_NAME"
#define CHILD_KEY_PORT2_NAME "PORT2_NAME"
#define CHILD_KEY_PORT3_NAME "PORT3_NAME"
#define CHILD_KEY_PORT4_NAME "PORT4_NAME"

#define CHILD_KEY_PORT1_BOOT "PORT1_BOOT"
#define CHILD_KEY_PORT2_BOOT "PORT2_BOOT"
#define CHILD_KEY_PORT3_BOOT "PORT3_BOOT"
#define CHILD_KEY_PORT4_BOOT "PORT4_BOOT"

#if defined(SB_WIN_BUILD)
#define DEF_PORT_NAME                    "COM1"
#elif defined(SB_MAC_BUILD)
#define DEF_PORT_NAME                    "/dev/cu.Bluetooth-Incoming-Port"
#elif defined(SB_LINUX_BUILD)
#define DEF_PORT_NAME                    "/dev/ttyUSB0"
#endif

class __attribute__((weak,visibility("default"))) X2PowerControl : public PowerControlDriverInterface, public MultiConnectionDeviceInterface,  public ModalSettingsDialogInterface, public X2GUIEventInterface, public CircuitLabelsInterface, public SetCircuitLabelsInterface, public SerialPortParams2Interface
{
public:
	X2PowerControl( const char* pszDisplayName,
                    const int& nInstanceIndex,
                    SerXInterface						* pSerXIn,
                    TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
                    SleeperInterface					* pSleeperIn,
                    BasicIniUtilInterface				* pIniUtilIn,
                    LoggerInterface						* pLoggerIn,
                    MutexInterface						* pIOMutexIn,
                    TickCountInterface					* pTickCountIn);

    virtual ~X2PowerControl();


public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{
	virtual DeviceType  deviceType(void) {return DriverRootInterface::DT_POWERCONTROL;}
	virtual int         queryAbstraction(const char* pszName, void** ppVal);
	//@}

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{
	virtual void        driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double      driverInfoVersion(void) const;
	//@}

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{
	virtual void deviceInfoNameShort(BasicStringInterface& str) const;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str);
	virtual void deviceInfoModel(BasicStringInterface& str);
	//@}

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{
	virtual int		establishLink(void);
	virtual int		terminateLink(void);
	virtual bool	isLinked(void) const;
	//@}

	virtual int		initModalSettingsDialog(void){return 0;}
	virtual int		execModalSettingsDialog(void);
	virtual void	uiEvent(X2GUIExchangeInterface*, const char*);

	//PowerControlDriverInterface
	virtual int		numberOfCircuits(int& nNumber);
	virtual int		circuitState(const int& nIndex, bool& bZeroForOffOneForOn);
	virtual int		setCircuitState(const int& nIndex, const bool& bZeroForOffOneForOn);

    virtual int     circuitLabel (const int &nZeroBasedIndex, BasicStringInterface &str);
    virtual int     setCircuitLabel	(const int &nZeroBasedIndex, const char *str);

    //SerialPortParams2Interface
    virtual void            portName(BasicStringInterface& str) const;
    virtual void            setPortName(const char* szPort);
    virtual unsigned int    baudRate() const            {return 9600;};
    virtual void            setBaudRate(unsigned int)    {};
    virtual bool            isBaudRateFixed() const        {return true;};

    virtual SerXInterface::Parity    parity() const                {return SerXInterface::B_NOPARITY;};
    virtual void                    setParity(const SerXInterface::Parity& parity){};
    virtual bool                    isParityFixed() const        {return true;};

    // MultiConnectionDeviceInterface
    virtual int deviceIdentifier(BasicStringInterface &sIdentifier);
    virtual int isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible);
    virtual int useResource(MultiConnectionDeviceInterface *pPeer);
    virtual int swapResource(MultiConnectionDeviceInterface *pPeer);

    SerXInterface*  m_pSavedSerX;
    MutexInterface* m_pSavedMutex;

    
private:

	//Standard device driver tools
	TheSkyXFacadeForDriversInterface*	m_pTheSkyXForMounts;
	SleeperInterface*                   m_pSleeper;
	BasicIniUtilInterface*              m_pIniUtil;
	LoggerInterface*                    m_pLogger;
	mutable MutexInterface*             m_pIOMutex;
	TickCountInterface*                 m_pTickCount;

	TheSkyXFacadeForDriversInterface    *GetTheSkyXFacadeForDrivers() { return m_pTheSkyXForMounts; }
	SleeperInterface                    *GetSleeper() { return m_pSleeper; }
	BasicIniUtilInterface               *GetSimpleIniUtil() { return m_pIniUtil; }
	LoggerInterface                     *GetLogger() { return m_pLogger; }
	MutexInterface                      *GetMutex() { return m_pIOMutex; }
	TickCountInterface                  *GetTickCountInterface() { return m_pTickCount; }

    void portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const;

	bool    m_bLinked;
	int	    m_nISIndex;
    
	CPegasusPPBAPower	m_PowerPorts;
    
    std::vector<std::string>    m_sPortNames;
    std::vector<std::string>    m_IniPortKey = {CHILD_KEY_PORT1_NAME, CHILD_KEY_PORT2_NAME, CHILD_KEY_PORT3_NAME, CHILD_KEY_PORT4_NAME};
};

#endif // __X2Power_H_
