#include "aircoolcontroll.h"
#include "Cooller_ModbusController.h"
#include "AddNewConfigWidget.h"
#include "EditConfigWindow.h"
#include "SelectConfigWidget.h"

AirCoolControll::AirCoolControll(QWidget *parent)
    : QMainWindow(parent),
    m_preferences(NULL),
    m_internetConnector(NULL)
{
    ui.setupUi(this);

    setWindowState(windowState() ^ Qt::WindowMaximized);
    m_uartConnector = new UART_ConnectionWindow(this);
    m_comunicator = new Cooller_ModBusController(this);
    m_uartDisconnector = new UART_DisconnectWindow(m_comunicator,this);
    m_uartConnector->setController(m_comunicator);
    
    connect(m_comunicator, SIGNAL(newStatus(const QString&)), this,SLOT(newStatus(const QString&)));

   ///  Toolbox actions  ///////////////////////////////////////////////////////////////

    connect(ui.actionPreferences, SIGNAL(triggered(void)), this, SLOT(showPreferencesDialog()));
    connect(ui.actionConnect_to_device, SIGNAL(triggered(void)), this, SLOT(showConnectDialog()));
    connect(ui.actionDisconnect_from_UART, SIGNAL(triggered(void)), this, SLOT(showDisconnectDialog()));
    connect(ui.actionConnect_to_host, SIGNAL(triggered(void)), this, SLOT(showConnectToHostDialog()));
    connect(ui.actionCreate_new_config, SIGNAL(triggered(void)), this, SLOT(showAddNewConfigDialog()));
    connect(ui.actionEdit_configuration, SIGNAL(triggered(void)), this, SLOT(showSelectConfigDialog()));

   /////////////////////////////////////////////////////////////////////////////////////
    connect(ui.mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(newActiveWindow(QMdiSubWindow*)));
}

AirCoolControll::~AirCoolControll()
{
   
}

QMdiArea*    AirCoolControll::getMdiArea(void) const
{
    return ui.mdiArea;
}

UART_ConnectionWindow * AirCoolControll::getUART_Configurator(void) const
{
    return m_uartConnector;
}

ConnectionLog * AirCoolControll::getConnectionLog(void) const
{
    return ui.deviceExplorerContent;
}

void AirCoolControll::showPreferencesDialog()
{
    if (NULL == m_preferences)
    {
        m_preferences = new PreferencesWindow(this);
    }
    QRect mainWindowSize = geometry();
    QRect connectorSize = m_preferences->geometry();
    m_preferences->move(mainWindowSize.x() + mainWindowSize.width() - connectorSize.width() - 20, mainWindowSize.y() + 50);
    
    m_preferences->exec();
}

void AirCoolControll::showConnectDialog()
{
    m_uartConnector->exec();
}

void AirCoolControll::showDisconnectDialog()
{
    m_uartDisconnector->exec();
}

void AirCoolControll::showConnectToHostDialog()
{
    if (m_internetConnector == NULL)
    {
        m_internetConnector = new ExternalConnectionWindow(this);   
    } 
    QSize mainWindowSize = ui.mdiArea->frameSize();
    QSize connectorSize = m_internetConnector->frameSize();

    m_internetConnector->exec();
}

void AirCoolControll::newStatus(const QString& statusString)
{
    ui.statusBar->showMessage(statusString);
}

void AirCoolControll::showAddNewConfigDialog(void)
{
    AddNewConfigWidget*  addConfig = new AddNewConfigWidget(m_comunicator->getConfigs(), this);
    
    int rc = addConfig->exec();
    if (1 == rc)
    {
        EditConfigWindow* edit = new EditConfigWindow(addConfig->getConfig(), this);
        edit->exec();
        delete edit;
    }

    delete addConfig;
}

void AirCoolControll::showSelectConfigDialog(void)
{
    SelectConfigWidget*  selectConfig = new SelectConfigWidget(m_comunicator->getConfigs(), this);

    int rc = selectConfig->exec();
    if (1 == rc)
    {
        ConfigMapShared config = selectConfig->getConfig();
        if (config)
        {
            EditConfigWindow* edit = new EditConfigWindow(config, this);
            edit->exec();
            delete edit;
        }
    }

    delete selectConfig;
}

void AirCoolControll::newActiveWindow(QMdiSubWindow* w)
{
    // TO DO activate current item in list
}