#include "OutValueWidget.h"
#include <algorithm>

OutValueWidget::OutValueWidget(DeviceExplorer* explorer, const std::string& name, const ConfigMap::Parameter& parameter, QWidget *parent)
    : ValueFieldWidget(explorer,name,parent),
    m_parameters(parameter),
    m_edit(NULL),
    m_combo(NULL)
{
    m_gridLayout = new QGridLayout(this);
    setLayout(m_gridLayout);
    
    if (m_parameters.m_enumeration.empty())
    {
        m_edit = new QSpinBox();
        m_edit->setMaximum(m_parameters.m_maxValue);
        m_edit->setMinimum(m_parameters.m_minValue);
        m_edit->setSingleStep(1);
        connect(m_edit, SIGNAL(valueChanged(int)), this, SLOT(registerSet(int)));
        m_gridLayout->addWidget(m_edit, 0, 0, 1, 1);
    }
    else
    {
        m_combo = new QComboBox();
        for (auto pair : m_parameters.m_enumeration)
        {
            m_combo->addItem(QString::fromStdString(pair.first), pair.second);
        }
        connect(m_combo, SIGNAL(activated(int)), this, SLOT(enumItemActivated(int)));
        m_gridLayout->addWidget(m_combo, 0, 0, 1, 1);
    }
}

OutValueWidget::~OutValueWidget()
{
    if (m_edit) delete m_edit;
    if (m_combo) delete m_combo;
    delete m_gridLayout;
}

void OutValueWidget::registerSet(int v)
{
    m_explorer->sendValueToDevice(ConfigMap::OUTPUT_REGISTER, QString::fromStdString(m_name), v);
}

void OutValueWidget::enumItemActivated(int v)
{
 //   int v = std::find_if(m_parameters.m_enumeration.begin(), m_parameters.m_enumeration.end(), 
 //       [&name](const std::pair<std::string, int>& p) {return name.toStdString() == p.first; })->second;
    registerSet(v);
}

void OutValueWidget::setValue(QVariant value)
{
    bool isValid;
    int number = value.toInt(&isValid);

    if (m_edit)
    {
        if (!m_edit->hasFocus())
        {
            m_edit->blockSignals(true);
            m_edit->setValue(number);
            m_edit->blockSignals(false);
        }
        m_edit->setGeometry(this->rect());
    }
    else
    {
        m_combo->blockSignals(true);
        bool error = true;
        for (int n = 0; n < m_parameters.m_enumeration.size(); n++)
        {
            if (m_parameters.m_enumeration[n].second == number)
            {
                m_combo->setCurrentIndex(n);
                error = false;
            }
        }
        if (error)
        {
            // TO DO add check
        }
        m_combo->setGeometry(this->rect());
        m_combo->blockSignals(false);
    }

    repaint();
}

void OutValueWidget::paintEvent(QPaintEvent *event)
{
    if (m_edit)
    {
        m_edit->setGeometry(this->rect());
    }
    else
    {
        m_combo->setGeometry(this->rect());
    }
}