#include "ModbusPuller.h"

ModbusPuller::ModbusPuller(QObject *parent)
    : QThread(parent),
    m_modbus(NULL),
    m_isStoped(true),
    m_continueProcessing(true),
    m_endProcessingSemaphore(1),
    m_removeIndex(0)
{
   
}

ModbusPuller::~ModbusPuller()
{
    m_continueProcessing = false;
    if(false == m_isStoped)
        m_endProcessingSemaphore.acquire(0);
}

void ModbusPuller::clearTaskList()
{
    QMutexLocker lock(&m_taskMutex);
    m_removeIndex = -1;
}

void ModbusPuller::removeTaskWithID(int id)
{
    QMutexLocker lock(&m_taskMutex);
    m_removeIndex = id;
}

void ModbusPuller::addTask(PullerTaskShared a_task)
{
    QMutexLocker lock(&m_taskMutex);
    m_newTasks.append(a_task);
}

void ModbusPuller::run(void)
{
    m_endProcessingSemaphore.acquire(1);
    int currentScanningID = 1;
    QString vendor, product, version;
    while (m_continueProcessing)
    {
        if (m_isStoped)
            QThread::yieldCurrentThread();

        for (QList<PullerTaskShared>::iterator task = m_tasks.begin(); task != m_tasks.end(); )
        {
            if ((*task)->isItTimeToDo() && true == (*task)->proceed(m_modbus))
            {
                task = m_tasks.erase(task);
            }
            else
            {
                task++;
            }
        }

        {
            QMutexLocker lock(&m_taskMutex);
            if (m_newTasks.size() != 0)
            {
                for (PullerTaskShared a_task : m_newTasks)
                    m_tasks.push_back(a_task);

                m_newTasks.clear();
            }
            if (0 != m_removeIndex)
            {
                if (-1 == m_removeIndex)
                    m_tasks.clear();
                else
                {
                    bool finished = true;
                    while (finished && m_tasks.size())
                    {
                        finished = false;
                        for (QList<PullerTaskShared>::iterator it = m_tasks.begin(); it != m_tasks.end(); it++)
                        {
                            if (m_removeIndex == (*it)->getID())
                            {

                                m_tasks.erase(it);
                                finished = true;
                                break;
                            }
                        }
                    }
                }
                m_removeIndex = 0;
            }
        }

    }
    m_endProcessingSemaphore.release(1);
}

void ModbusPuller::startPulling(ModBusUART_Impl* modbus)
{
    m_modbus = modbus;
    m_isStoped = false;
}

void ModbusPuller::stopPulling()
{
    m_isStoped = true;
}