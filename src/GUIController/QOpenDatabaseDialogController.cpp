/*
 * QOpenDatabaseDialogController.cpp
 *
 *  Created on: 10 juin 2016
 *      Author: echopin
 */

#include <QFileDialog>

#include "GUI/QOpenDatabaseDialog.h"
#include "GUI/QDatabaseConnectionView.h"
#include "GUI/QWindowMain.h"

#include "QDatabaseConnectionViewController.h"
#include <GUIController/QOpenDatabaseDialogController.h>
#include "Database/DatabaseController.h"
#include "Database/DatabaseControllerMysql.h"
#include "Database/DatabaseControllerSqlite.h"

QOpenDatabaseDialogController::QOpenDatabaseDialogController()
{
	m_pOpenDatabaseDialog = NULL;
	m_pMainWindow = NULL;
}

QOpenDatabaseDialogController::~QOpenDatabaseDialogController()
{

}

void QOpenDatabaseDialogController::init(QWindowMain* pMainWindow, QOpenDatabaseDialog* pOpenDatabaseDialog)
{
	m_pMainWindow = pMainWindow;
	m_pOpenDatabaseDialog = pOpenDatabaseDialog;

	connect(m_pOpenDatabaseDialog->getFileSelectionButton(), SIGNAL(clicked()), this, SLOT(openFileDialog()));
	connect(m_pOpenDatabaseDialog->getCancelButton(), SIGNAL(clicked()), this, SLOT(closeOpenDatabaseDialog()));
	connect(m_pOpenDatabaseDialog->getOKButton(), SIGNAL(clicked()), this, SLOT(loadDatabase()));
}


void QOpenDatabaseDialogController::openFileDialog()
{
	m_fileName = QFileDialog::getOpenFileName(m_pOpenDatabaseDialog, tr("Select a file"), QString(), tr("SQLite files (*.sqlite *.db)"));

	m_pOpenDatabaseDialog->getFilePathField()->setText(m_fileName);
}

void QOpenDatabaseDialogController::closeOpenDatabaseDialog()
{
	m_pOpenDatabaseDialog->close();
}

void QOpenDatabaseDialogController::loadDatabase()
{
	// Creating a string to be used to set the tab name
	QString szTabFileName;
	QStringList szDatabaseInfoList;

	bool bGoOn = true;
	QString szErrorMsg;

	DatabaseController* pDatabaseController = NULL;
	QDatabaseConnectionView* pConnectionView = NULL;
	QDatabaseConnectionViewController* pDatabaseConnectionViewController = NULL;

	int iCurrentIndex = m_pOpenDatabaseDialog->getConnectionTypeTabWidget()->currentIndex();

	switch(iCurrentIndex){
	case 0: // SQLite
		bGoOn = !m_fileName.isEmpty();
		if(bGoOn){
			szTabFileName =	m_fileName.section('/', -1);//Get the last part of the file path to get the name for the tab
			pDatabaseController = new DatabaseControllerSqlite(m_fileName);
		}else{
			szErrorMsg = tr("Please select a valid SQLite file");
		}
		break;
	case 1: // MySQL
		bGoOn = !m_pOpenDatabaseDialog->getHostField()->text().isEmpty() && !m_pOpenDatabaseDialog->getDatabaseField()->text().isEmpty();
		if(bGoOn){
			szTabFileName = m_pOpenDatabaseDialog->getDatabaseField()->text();
			szDatabaseInfoList = makeDatabaseInfoList();
			pDatabaseController = new DatabaseControllerMysql(m_fileName, szDatabaseInfoList);
		}else{
			szErrorMsg = tr("Please enter the necessary information.");
		}
		break;
	default:
		break;
	}

	// Testing the connection, if it fails, give a warning
	if(bGoOn){
		bGoOn = pDatabaseController->openDatabase();
		if(bGoOn){
			pDatabaseController->closeDataBase();
		}else{
			szErrorMsg = tr("Unable to connect to the database, please check the connection information.");
		}
	}

	// Init the view and fill it
	if(bGoOn){
		pConnectionView = new QDatabaseConnectionView(m_pMainWindow);
		pDatabaseConnectionViewController = new QDatabaseConnectionViewController(m_fileName, pDatabaseController);
		pDatabaseConnectionViewController->init(pConnectionView, szDatabaseInfoList);

		// Adding DatabaseConnectionView to the main window
		m_pMainWindow->addDatabaseConnectionView(pConnectionView, szTabFileName);

		// Updating tables
		if(!pDatabaseConnectionViewController->loadDatabaseTables()){
			QMessageBox::warning(m_pOpenDatabaseDialog, tr("Database problem"), tr("Problem while loading database tables"));
		}

		// Controller will be deleted when the view is destroyed
		connect(pConnectionView, SIGNAL(destroyed(QObject*)), pDatabaseConnectionViewController, SLOT(deleteLater()));
	}

	if(!bGoOn){
		if(pDatabaseController){
			delete pDatabaseController;
			pDatabaseController = NULL;
		}

		QMessageBox::critical(m_pOpenDatabaseDialog, tr("Connection error"), szErrorMsg);
	}

	if(bGoOn){
		closeOpenDatabaseDialog();
	}
}

QString QOpenDatabaseDialogController::getFileName() const
{
	return m_fileName;
}

QStringList QOpenDatabaseDialogController::makeDatabaseInfoList()
{
	QStringList szDatabaseInfoList;

	szDatabaseInfoList << m_pOpenDatabaseDialog->getHostField()->text();
	szDatabaseInfoList << m_pOpenDatabaseDialog->getPortField()->text();
	szDatabaseInfoList << m_pOpenDatabaseDialog->getUsernameField()->text();
	szDatabaseInfoList << m_pOpenDatabaseDialog->getPasswordField()->text();
	szDatabaseInfoList << m_pOpenDatabaseDialog->getDatabaseField()->text();

	return szDatabaseInfoList;
}
