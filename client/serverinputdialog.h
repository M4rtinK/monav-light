#ifndef SERVERINPUTDIALOG_H
#define SERVERINPUTDIALOG_H

#include <QDialog>
#include <QList>
#include <QStandardItemModel>

#include "serverlogic.h"

namespace Ui {
    class ServerInputDialog;
}

class ServerInputDialog : public QDialog
{
    Q_OBJECT

public:

	explicit ServerInputDialog(const QVector< ServerLogic::Server > &servers, QWidget *parent = 0);
    ~ServerInputDialog();

	void writeServerSettings(QVector< ServerLogic::Server > *servers);

protected:
	Ui::ServerInputDialog *m_ui;

protected slots:
	void addServer();
	void removeServer();
};

#endif // SERVERINPUTDIALOG_H
