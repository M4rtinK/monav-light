#include "activationchangeeventfilter.h"

ActivationChangeEventFilter::ActivationChangeEventFilter(QObject *parent) : QObject(parent) {
}
 
ActivationChangeEventFilter::~ActivationChangeEventFilter() {
}
 
bool ActivationChangeEventFilter::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::ApplicationActivate) {
		emit applicationActive();
		return false;
	} else {
		return QObject::eventFilter(obj, event);
	}
}
