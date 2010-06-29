/*
 * Copyright (C) 2010 Rafael Fernández López <ereslibre@kde.org>
 * Copyright (C) 2010 UFO Coders <info@ufocoders.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "bluedeviladapters.h"
#include "systemcheck.h"

#include <QtCore/QTimer>

#include <QtGui/QScrollArea>
#include <QtGui/QBoxLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QSlider>
#include <QtGui/QLabel>
#include <QtGui/QFormLayout>
#include <QtGui/QButtonGroup>

#include <bluedevil/bluedevil.h>

#include <klineedit.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>

K_PLUGIN_FACTORY(BlueDevilFactory, registerPlugin<KCMBlueDevilAdapters>();)
K_EXPORT_PLUGIN(BlueDevilFactory("bluedeviladapters"))

////////////////////////////////////////////////////////////////////////////////////////////////////

AdapterSettings::AdapterSettings(Adapter *adapter, KCModule *parent)
    : QGroupBox(parent)
    , m_adapter(adapter)
    , m_name(new KLineEdit(this))
    , m_hidden(new QRadioButton(i18n("Hidden"), this))
    , m_alwaysVisible(new QRadioButton(i18n("Always visible"), this))
    , m_temporaryVisible(new QRadioButton(i18n("Temporary visible"), this))
    , m_discoverTime(new QSlider(Qt::Horizontal, this))
    , m_discoverTimeLabel(new QLabel(this))
    , m_discoverTimeWidget(new QWidget(this))
    , m_powered(new QCheckBox(i18n("Powered"), this))
{
    QButtonGroup *const buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(m_hidden);
    buttonGroup->addButton(m_alwaysVisible);
    buttonGroup->addButton(m_temporaryVisible);

    m_name->setText(adapter->name());
    m_nameOrig = adapter->name();
    m_hiddenOrig = false;
    m_alwaysVisibleOrig = false;
    m_temporaryVisibleOrig = false;
    if (!adapter->isDiscoverable()) {
        m_hidden->setChecked(true);
        m_hiddenOrig = true;
    } else {
        if (!adapter->discoverableTimeout()) {
            m_alwaysVisible->setChecked(true);
            m_alwaysVisibleOrig = true;
        } else {
            m_temporaryVisible->setChecked(true);
            m_temporaryVisibleOrig = true;
        }
    }
    m_discoverTime->setRange(1, 10);
    m_discoverTime->setValue(adapter->discoverableTimeout() / 60);
    m_discoverTime->setTickPosition(QSlider::TicksBelow);
    m_discoverTime->setTickInterval(1);
    m_discoverTimeOrig = adapter->discoverableTimeout() / 60;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_discoverTime);
    layout->addWidget(m_discoverTimeLabel);
    m_discoverTimeWidget->setLayout(layout);
    m_discoverTimeWidget->setEnabled(m_temporaryVisibleOrig);

    m_discoverTimeLabel->setText(i18np("1 minute", "%1 minutes", m_discoverTime->value()));

    m_powered->setChecked(adapter->isPowered());
    m_poweredOrig = adapter->isPowered();

    m_layout = new QFormLayout;
    m_layout->addRow(i18n("Name"), m_name);
    m_layout->addWidget(m_powered);
    m_layout->addRow(i18n("Visibility"), m_hidden);
    m_layout->addWidget(m_alwaysVisible);
    m_layout->addWidget(m_temporaryVisible);
    m_layout->addRow(i18n("Discover Time"), m_discoverTimeWidget);
    setLayout(m_layout);

    m_layout->labelForField(m_discoverTimeWidget)->setEnabled(m_temporaryVisibleOrig);

    connect(m_name, SIGNAL(textEdited(QString)), this, SLOT(slotSettingsChanged()));
    connect(m_hidden, SIGNAL(toggled(bool)), this, SLOT(visibilityChanged()));
    connect(m_hidden, SIGNAL(toggled(bool)), this, SLOT(slotSettingsChanged()));
    connect(m_alwaysVisible, SIGNAL(toggled(bool)), this, SLOT(visibilityChanged()));
    connect(m_alwaysVisible, SIGNAL(toggled(bool)), this, SLOT(slotSettingsChanged()));
    connect(m_temporaryVisible, SIGNAL(toggled(bool)), this, SLOT(visibilityChanged()));
    connect(m_temporaryVisible, SIGNAL(toggled(bool)), this, SLOT(slotSettingsChanged()));
    connect(m_discoverTime, SIGNAL(valueChanged(int)), this, SLOT(slotSettingsChanged()));
    connect(m_powered, SIGNAL(stateChanged(int)), this, SLOT(slotSettingsChanged()));

    setTitle(i18n("Adapter: %1 (%2)").arg(adapter->name()).arg(adapter->address()));
}

AdapterSettings::~AdapterSettings()
{
}

bool AdapterSettings::isModified() const
{
    return m_name->text() != m_nameOrig || m_hidden->isChecked() != m_hiddenOrig ||
           m_alwaysVisible->isChecked() != m_alwaysVisibleOrig ||
           m_temporaryVisible->isChecked() != m_temporaryVisibleOrig ||
           m_discoverTime->value() != m_discoverTimeOrig || m_powered->isChecked() != m_poweredOrig;
}

QString AdapterSettings::name() const
{
    return m_name->text();
}

AdapterSettings::DiscoverOptions AdapterSettings::discoverOptions() const
{
    if (m_hidden->isChecked()) {
        return Hidden;
    }
    if (m_alwaysVisible->isChecked()) {
        return AlwaysVisible;
    }
    return TemporaryVisible;
}

quint32 AdapterSettings::discoverTime() const
{
    return m_discoverTime->value() * 60;
}

bool AdapterSettings::powered() const
{
    return m_powered->isChecked();
}

void AdapterSettings::visibilityChanged()
{
    QRadioButton *const sdr = static_cast<QRadioButton*>(sender());
    if (!sdr->isChecked()) {
        return;
    }
    const bool enabled = sender() == m_temporaryVisible;
    m_discoverTimeWidget->setEnabled(enabled);
    m_layout->labelForField(m_discoverTimeWidget)->setEnabled(enabled);
}

void AdapterSettings::slotSettingsChanged()
{
    m_discoverTimeLabel->setText(i18np("1 minute", "%1 minutes", m_discoverTime->value()));
    emit settingsChanged(isModified());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

KCMBlueDevilAdapters::KCMBlueDevilAdapters(QWidget *parent, const QVariantList&)
    : KCModule(BlueDevilFactory::componentData(), parent)
    , m_systemCheck(new SystemCheck(this))
{
    KAboutData* ab = new KAboutData(
        "kcmbluedeviladapters", 0, ki18n("BlueDevil Adapters"), "1.0",
        ki18n("BlueDevil Adapters Control Panel Module"),
        KAboutData::License_GPL, ki18n("(c) 2010 Rafael Fernández López"));

    ab->addAuthor(ki18n("Rafael Fernández López"), ki18n("Developer and Maintainer"), "ereslibre@kde.org");
    setAboutData(ab);

    connect(m_systemCheck, SIGNAL(updateInformationStateRequest()),
            this, SLOT(updateInformationState()));

    QVBoxLayout *layout = new QVBoxLayout;
    QScrollArea *mainArea = new QScrollArea(this);
    QWidget *widget = new QWidget(mainArea);
    m_layout = new QVBoxLayout;
    widget->setLayout(m_layout);
    mainArea->setWidget(widget);
    mainArea->setWidgetResizable(true);
    layout->addWidget(mainArea);
    setLayout(layout);

    fillAdaptersInformation();
}

KCMBlueDevilAdapters::~KCMBlueDevilAdapters()
{
}

void KCMBlueDevilAdapters::defaults()
{
}

void KCMBlueDevilAdapters::save()
{
}

void KCMBlueDevilAdapters::defaultAdapterChanged(Adapter *adapter)
{
    if (adapter) {
        connect(adapter, SIGNAL(discoverableChanged(bool)),
                this, SLOT(adapterDiscoverableChanged()));
        connect(adapter, SIGNAL(devicesChanged(QList<Device*>)),
                this, SLOT(adapterDevicesChanged(QList<Device*>)));
    }
    QTimer::singleShot(300, this, SLOT(updateInformationState()));
}

void KCMBlueDevilAdapters::adapterDiscoverableChanged()
{
    QTimer::singleShot(300, this, SLOT(updateInformationState()));
}

void KCMBlueDevilAdapters::updateInformationState()
{
    m_systemCheck->updateInformationState();
}

void KCMBlueDevilAdapters::adapterConfigurationChanged(bool modified)
{
    if (modified) {
        emit changed(true);
        return;
    }
    Q_FOREACH (AdapterSettings *const adapterSettings, m_adapterSettingsMap) {
        if (adapterSettings->isModified()) {
            return;
        }
    }
    emit changed(false);
}

void KCMBlueDevilAdapters::fillAdaptersInformation()
{
    Q_FOREACH (Adapter *const adapter, BlueDevil::Manager::self()->adapters()) {
        AdapterSettings *const adapterSettings = new AdapterSettings(adapter, this);
        connect(adapterSettings, SIGNAL(settingsChanged(bool)),
                this, SLOT(adapterConfigurationChanged(bool)));
        m_adapterSettingsMap.insert(adapter, adapterSettings);
        m_layout->addWidget(adapterSettings);
    }
}
