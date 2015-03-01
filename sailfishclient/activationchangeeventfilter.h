#ifndef ACTIVATIONCHANGEEVENTFILTER
#define ACTIVATIONCHANGEEVENTFILTER

#include <QObject>
#include <QEvent>

class ActivationChangeEventFilter : public QObject {
	Q_OBJECT
public:
	ActivationChangeEventFilter(QObject *parent = 0);
	~ActivationChangeEventFilter();
protected:
	bool eventFilter(QObject *obj, QEvent *event);
signals:
	void applicationActive();
};

#endif
