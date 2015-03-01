#ifndef TAPMENU_H
#define TAPMENU_H

#include <QObject>
#include <QStringList>
#include <QVector>

class TapMenu : public QObject {
	Q_OBJECT
public:
	TapMenu(QObject *parent = 0);
	~TapMenu();
public slots:
	void searchTextChanged(QString text);
	void searchResultSelected(QString command, int index);
	void setSourceFollowLocation(bool follow);
signals:
	void searchResultUpdated(QStringList list, QStringList placeNames);
private:
	QStringList search_suggestions;
	QVector<unsigned> search_dataIndex;
};

#endif
