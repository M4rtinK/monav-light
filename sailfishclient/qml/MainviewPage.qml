import QtQuick 2.0
import harbour.monav 1.0
import Sailfish.Silica 1.0

Page {
	id: mainviewpage
	objectName: "mainviewpage"
	allowedOrientations: Orientation.All
	property alias mainview : mainview
	property alias paintwidget : paintwidget
	property alias instructions : instructions
	property bool navigation: false
	
	Mainview {
		id: mainview
		objectName: "mainview"
		
		onInstructionsLoaded: {
			mainviewpage.instructions.update(label, icon);
		}
		
		onMapDataLoadFailed: {
			pageStack.completeAnimation();
			pagestackbusywaittimer.restart();
		}
	}
	
	Timer {
		id: pagestackbusywaittimer
		interval: 500
		onTriggered: {
			if (pageStack.busy) {
				pagestackbusywaittimer.restart();
			} else {
				pageStack.push(Qt.resolvedUrl("MapPackagesPage.qml"));
			}
		}
	}

	SilicaFlickable {
		anchors.fill: parent
		interactive: navigation
		
		PullDownMenu {
			visible: navigation
			MenuItem {
				text: "Stop navigation"
				onClicked: {
					mainview.setNavigation(false);
					navigation = false;
				}
			}
		}
		
		PaintWidget {
			id: paintwidget
			objectName: "paintwidget"
			anchors.fill: parent

			onMouseClicked: pageStack.push(Qt.resolvedUrl("TapMenuPage.qml"))

			MultiPointTouchArea {
				maximumTouchPoints: 2
				anchors.fill: parent
				onPressed: {
					var list = [];
					var idlist = [];
					for (var i = 0; i < touchPoints.length; i++) {
						list.push(Qt.point(touchPoints[i].x, touchPoints[i].y));
						idlist.push(touchPoints[i].pointId);
					}
					parent.press(list, idlist);
				}
				onTouchUpdated: {
					var list = [];
					var idlist = [];
					for (var i = 0; i < touchPoints.length; i++) {
						list.push(Qt.point(touchPoints[i].x, touchPoints[i].y));
						idlist.push(touchPoints[i].pointId);
					}
					parent.move(list, idlist)
				}
				onReleased: {
					var list = [];
					var idlist = [];
					for (var i = 0; i < touchPoints.length; i++) {
						list.push(Qt.point(touchPoints[i].x, touchPoints[i].y));
						idlist.push(touchPoints[i].pointId);
					}
					parent.release(list, idlist)
				}
			}
		}
		
		Grid {
			id: instructions
			columns: 2
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}
			Image {
				id: routingicon1
				source: "qrc:///images/directions/forward.png"
				visible: routinglabel1.visible
			}
			Label {
				id: routinglabel1
				text: "test"
				color: "black"
			}
			Image {
				id: routingicon2
				source: "qrc:///images/directions/forward.png"
				visible: routinglabel2.visible
			}
			Label {
				id: routinglabel2
				text: "test2"
				color: "black"
			}
			
			function update(label, icon) {
				routinglabel1.visible = label.length >= 1;
				routinglabel2.visible = label.length >= 2;
				if (label.length >= 1) {
					routinglabel1.text = label[0];
					routingicon1.source = "qrc://" + icon[0].substr(1);
				}
				if (label.length >= 2) {
					routinglabel2.text = label[1];
					routingicon2.source = "qrc://" + icon[1].substr(1);
				}
			}
		}

		Row {
			x: 0
			y: parent.height - 50
			width: parent.width
			height: 50
			Button {
				width: parent.width/2
				text: "+"
				color: "black"

				onClicked: {
					mainview.addZoom();
				}
			}
			Button {
				width: parent.width/2
				text: "-"
				color: "black"

				onClicked: {
					mainview.subtractZoom();
				}
			}
		}
	}
	
	onStatusChanged: {
		if (status == PageStatus.Activating) {
			mainview.pageActivating();
		}
	}
}

