/*
 * This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2010 Alejandro Fiestas Olivares <afiestas@kde.org>
 * SPDX-FileCopyrightText: 2010-2011 UFO Coders <info@ufocoders.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef FAIL_H
#define FAIL_H

#include "ui_fail.h"

#include <QWizardPage>

class BlueWizard;

class FailPage : public QWizardPage, Ui::Fail
{
    Q_OBJECT

public:
    explicit FailPage(BlueWizard *parent = nullptr);

    void initializePage() override;

private:
    BlueWizard *m_wizard;
};

#endif // FAIL_H
