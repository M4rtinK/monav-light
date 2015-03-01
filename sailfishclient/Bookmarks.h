#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QObject>
#include <QStringList>

class Bookmarks : public QObject {
	Q_OBJECT
	Q_PROPERTY(QStringList bookmarks READ getBookmarks)
public:
	Bookmarks(QObject *parent = 0);
	~Bookmarks();
public slots:
	void addBookmark(QString name);
	void setBookmark(int index);
	void delBookmark(int index);
private:
	QStringList getBookmarks();
};

#endif
