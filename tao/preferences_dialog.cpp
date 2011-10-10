// ****************************************************************************
//  preferences_dialog.cpp                                         Tao project
// ****************************************************************************
//
//   File Description:
//
//     PreferencesDialog implementation.
//
//
//
//
//
//
//
//
// ****************************************************************************
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include <QtGui>

#include "preferences_dialog.h"
#include "preferences_pages.h"

namespace Tao {

PreferencesDialog::PreferencesDialog(QWidget *parent)
// ----------------------------------------------------------------------------
//   Create the preference dialog
// ----------------------------------------------------------------------------
    : QDialog(parent)
{
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 96));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMinimumHeight(280);
    contentsWidget->setMaximumWidth(140);
    contentsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(new GeneralPage);
#ifndef CFG_NOMODPREF
    pagesWidget->addWidget(new ModulesPage);
#endif
    pagesWidget->addWidget(new PerformancesPage);
#ifdef DEBUG
    pagesWidget->addWidget(new DebugPage);
#endif

    QPushButton *closeButton = new QPushButton(tr("Close"));
    closeButton->setDefault(true);

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Tao Preferences"));
}


void PreferencesDialog::createIcons()
// ----------------------------------------------------------------------------
//   Add one icon per preference page
// ----------------------------------------------------------------------------
{
    // general.png downloaded from:
    // http://www.iconfinder.com/icondetails/17814/128/preferences_settings_tools_icon
    // Author: Everaldo Coelho (Crystal Project)
    // License: LGPL
    QListWidgetItem *generalButton = new QListWidgetItem(contentsWidget);
    generalButton->setIcon(QIcon(":/images/general.png"));
    generalButton->setText(tr("General options"));
    generalButton->setTextAlignment(Qt::AlignHCenter);
    generalButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

#ifndef CFG_NOMODPREF
    // modules.png downloaded from:
    // http://www.iconfinder.com/icondetails/17854/128/cubes_modules_icon
    // Author: Everaldo Coelho (Crystal Project)
    // License: LGPL
    QListWidgetItem *modulesButton = new QListWidgetItem(contentsWidget);
    modulesButton->setIcon(QIcon(":/images/modules.png"));
    modulesButton->setText(tr("Module options"));
    modulesButton->setTextAlignment(Qt::AlignHCenter);
    modulesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
#endif

    // performances.png downloaded from:
    // http://www.iconfinder.com/icondetails/47542/128/performance_settings_speed_icon
    // Author: webiconset.com
    // License: Free for commercial use (Do not redistribute)
    QListWidgetItem *perfButton = new QListWidgetItem(contentsWidget);
    perfButton->setIcon(QIcon(":/images/performances.png"));
    perfButton->setText(tr("Performances"));
    perfButton->setTextAlignment(Qt::AlignHCenter);
    perfButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

#ifdef DEBUG
    // bug.png downloaded from:
    // http://www.iconfinder.com/icondetails/17857/128/animal_bug_insect_ladybird_icon
    // Author: Everaldo Coelho (Crystal Project)
    // License: LGPL
    QListWidgetItem *debugButton = new QListWidgetItem(contentsWidget);
    debugButton->setIcon(QIcon(":/images/bug.png"));
    debugButton->setText(tr("Debug options"));
    debugButton->setTextAlignment(Qt::AlignHCenter);
    debugButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
#endif

    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
}


void PreferencesDialog::changePage(QListWidgetItem *current,
                                   QListWidgetItem *previous)
// ----------------------------------------------------------------------------
//   Change current page when icon is clicked in contents widget
// ----------------------------------------------------------------------------
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

}
