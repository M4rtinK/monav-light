import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
	id: coverpage
	objectName: "coverpage"
	property alias placeholder : placeholder
	property alias instructions : instructions
	
	CoverPlaceholder {
		id: placeholder
		text: "MoNav"
		icon.source: "qrc:///images/source.png"
		visible: !instructions.visible
	}
	
	Column {
		id: instructions
		visible: false
		anchors {
			left: parent.left
			right: parent.right
		}
		y: parent.height / 2 - height / 2
		Image {
			id: routingicon1
			source: "qrc:///images/directions/forward.png"
			x: parent.width / 2 - width / 2
			visible: routinglabel1.visible
		}
		Label {
			id: routinglabel1
			text: "test"
			anchors {
				left: parent.left
				right: parent.right
			}
			horizontalAlignment: Text.AlignHCenter
			wrapMode: Text.WordWrap
		}
		Image {
			id: routingicon2
			source: "qrc:///images/directions/forward.png"
			x: parent.width / 2 - width / 2
			visible: routinglabel2.visible
		}
		Label {
			id: routinglabel2
			text: "test2"
			anchors {
				left: parent.left
				right: parent.right
			}
			horizontalAlignment: Text.AlignHCenter
			wrapMode: Text.WordWrap
		}
		
		function update(label, icon) {
			instructions.visible = label.length >= 1;
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
	
	Component.onCompleted: {
		var mainviewpage = pageStack.find(function (page) {
			return page.objectName == "mainviewpage";
		});
		mainviewpage.mainview.instructionsLoaded.connect(instructions.update);
	}
}
