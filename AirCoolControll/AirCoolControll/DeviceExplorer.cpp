#include <cfloat>
#include <qmdiarea.h>
#include "DeviceExplorer.h"

DeviceExplorer::DeviceExplorer(const ConfigMapShared config, ModbusDriverShared modbus, DeviceInfoShared info, QObject *parent)
    : QObject(parent),
    m_info(*info),
    m_modbus(modbus),
    m_currentMap(config),
    m_errorString(tr("Working")),
    m_mdi(NULL),
    m_listView(NULL)
{
    m_view = new CoolerStateWidget(this);
    int regCount = 0;
    
    for (ConfigMap::RegisterType i = ConfigMap::REGISTER_PULL_FIRST; i < ConfigMap::REGISTER_PULL_COUNT; ConfigMap::NEXT(i))
    {
        Interval a_int = m_currentMap->getInterval(i);
        if (!a_int.empty())
        {
            m_registers[i] = (i != ConfigMap::COIL) ? std::make_shared<PullerReadTask>(m_info.getID(), m_info.getSpeed(), a_int) :
                                                      std::make_shared<PullerReadCoilTask>(m_info.getID(), m_info.getSpeed(), a_int);
            m_modbus->addPullerReadTask(m_registers[i]);
            m_localPull[i].resize(a_int.second - a_int.first + 1);
            
            m_registers[i]->setListener(this);
            regCount += a_int.length();
        }
    }
    m_view->setParameterList(m_currentMap);
}

DeviceExplorer::~DeviceExplorer()
{
    stopTasks();
    if (m_mdi)
        delete m_mdi;
    else
        delete m_view;
}

void DeviceExplorer::stopTasks()
{
    m_modbus->removeTaskWithID(m_info.getID());
}

QVariant  DeviceExplorer::getRegisterValue(const std::string & key)
{
    ConfigMap::RegisterType a_type = m_currentMap->getVariableType(key);
    if (m_registers[a_type] == NULL)
        return QVariant(QString(QObject::tr("undefined")));

    return m_currentMap->getValue(key, m_localPull[a_type]);
}

void DeviceExplorer::setRegisterValue(const std::string & key,int value)
{
    if ( m_currentMap->haveVariableWithName(key))
    {
        int bitNumber;
        if (m_currentMap->isVariableBool(key, bitNumber))
        {
            bool isValid;
            int currentValue = getRegisterValue(key).toInt(&isValid);
            if (!isValid)
                return;
            value = (bool)value ? currentValue | (1 << bitNumber) : currentValue & ~(1 << bitNumber);
        }
        m_modbus->writeRegister(m_info.getID(), m_info.getSpeed(), m_currentMap->getRegisterNumber(key), value);
    }
}

void  DeviceExplorer::setCoilState(const std::string & key, bool state)
{
    if (m_currentMap->haveVariableWithName(key))
    {
        m_modbus->setCoil(m_info.getID(), m_info.getSpeed(), m_currentMap->getRegisterNumber(key), state);
    }
}

QString DeviceExplorer::errorString()
{
    return m_errorString;
}


void DeviceExplorer::sendValueToDevice(int registerType, QString& name, int v)
{
    switch (static_cast<ConfigMap::RegisterType>(registerType))
    {
    case ConfigMap::OUTPUT_REGISTER:
        setRegisterValue(name.toStdString(), v);
        break;
    case ConfigMap::COIL:
        setCoilState(name.toStdString(), static_cast<bool>(v));
        break;
    }
}

void DeviceExplorer::updateStateWidget()
{
    for (ConfigMap::RegisterType it = ConfigMap::REGISTER_PULL_FIRST; it < ConfigMap::REGISTER_PULL_COUNT; ConfigMap::NEXT(it))
    {
        const ConfigMap::ParameterList& parameters = m_currentMap->getParametersList(it);

        for (int i = 0; i < parameters.size(); i++)
        {
            m_view->updateParameter(i, getRegisterValue(parameters[i].first), it);
        }
    }
}

void DeviceExplorer::activateView(QMdiArea * area)
{
    if (NULL == m_mdi)
    {
        m_mdi = new MdiSubWindowPermanent(m_view,area);
        m_mdi->setWindowTitle(m_info.getDescription());
        connect(m_mdi, SIGNAL(windowStateChanged(Qt::WindowStates, Qt::WindowStates)), this, SLOT(viewStateChanged(Qt::WindowStates, Qt::WindowStates)));
        area->addSubWindow(m_mdi);
        m_listView->updateContent();
    }
    m_mdi->show();
    m_mdi->activateWindow();
    m_mdi->showNormal();
}

void DeviceExplorer::minimizeView(QMdiArea * area)
{
    m_mdi->showMinimized();
}

void DeviceExplorer::somethingChanged()
{
    for (ConfigMap::RegisterType i = ConfigMap::REGISTER_PULL_FIRST; i < ConfigMap::REGISTER_PULL_COUNT; ConfigMap::NEXT(i))
    {
        if (m_registers[i]->isContentChanged())
        {
            m_registers[i]->getContent(m_localPull[i]);
     
            const ConfigMap::ParameterList& parameters = m_currentMap->getParametersList(i);
            for (auto& it : parameters)
            {
                QVariant value = m_currentMap->getValue(it.first, m_localPull[i]);
                bool isCorrect;
                qint16 digit = value.toInt(&isCorrect);
                if (!isCorrect)
                    digit = 0x8000;
                m_history.addValue(QString::fromStdString(it.first), digit);
            }
        }
    }

    updateStateWidget();
}

void DeviceExplorer::viewStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
{
    if (newState == Qt::WindowActive && m_listView)
    {
        m_listView->activateDevice(this);
    }
}

void DeviceExplorer::setListView(ConnectionLog* view)
{
    m_listView = view;
}

void DeviceExplorer::getHistoryForRegesty(const QString& name, QVector<qreal>& timeLabels, QVector<qreal>& values)
{
    m_history.getOneHistory(name, timeLabels, values);
}