import QtQuick 2.0
import harbour.monav 1.0
import Sailfish.Silica 1.0

Page {
	id: tapmenupage
	objectName: "tapmenupage"
	property alias tapmenu: tapmenu
	property alias findlist: findlist
	
	TapMenu {
		id: tapmenu
		objectName: "tapmenu"
		
		onSearchResultUpdated: {
			tapmenupage.findlist.update(list, placeNames);
		}
	}
	
	SilicaFlickable {
		id: flick
		anchors.fill: parent
		contentHeight: searchfield.height + findlist.height + view.height
	
		SearchField {
			width: parent.width
			id: searchfield
			placeholderText: "Find an address"
			
			onTextChanged: {
				tapmenu.searchTextChanged(text)
			}
		}
		
		SilicaListView {
			id: findlist
			interactive: false
			property Item contextMenu
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: searchfield.bottom
			height: childrenRect.height
			
			model: ListModel {
				id: searchresults
			}
			
			delegate: ListItem {
				id: listitem
				property alias label: label
				property bool menuOpen: findlist.contextMenu != null && findlist.contextMenu.parent === listitem
				height: menuOpen ? findlist.contextMenu.height + Theme.itemSizeSmall : Theme.itemSizeSmall
				Label {
					id: label
					text: itemtext
					anchors {
						left: parent.left
						right: parent.right
						//margins: Theme.paddingLarge
						leftMargin: Theme.paddingLarge
					}
				}
				
				Label {
					id: label2
					text: itemtext2
					font.pixelSize: Theme.fontSizeSmall
					color: Theme.secondaryColor
					anchors {
						left: parent.left
						right: parent.right
						//margins: Theme.paddingLarge
						leftMargin: Theme.paddingLarge
						top: label.bottom
					}
				}
				
				onClicked: {
					if (!findlist.contextMenu) {
						findlist.contextMenu = contextMenuComponent.createObject(findlist)
					}
					findlist.contextMenu.show(listitem)
				}
				
				function getIndex() {
					return index;
				}
			}
			
			function update(list, placeNames) {
				searchresults.clear();
				
				for (var i = 0; i < list.length; i++) {
					searchresults.append({itemtext: list[i], itemtext2: placeNames[i]});
				}
			}
			
			Component {
				id: contextMenuComponent
				ContextMenu {
					MenuItem {
						text: "Set as destination"
						onClicked: {
							tapmenu.searchResultSelected("destination", findlist.contextMenu.parent.getIndex());
							pageStack.pop()
						}
					}
					MenuItem {
						text: "Set as source"
						onClicked: {
							tapmenu.searchResultSelected("source", findlist.contextMenu.parent.getIndex());
							pageStack.pop()
						}
					}
				}
			}
		}

		SilicaListView {
			interactive: false
			anchors.top: findlist.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			height: childrenRect.height
			id: view
			model: ListModel {
				ListElement { itemtext: "Destination"; itemicon: "qrc:///images/target.png" }
				ListElement { itemtext: "Source"; itemicon: "qrc:///images/source.png" }
				ListElement { itemtext: "Start Navigation"; itemicon: "qrc:///images/route.png" }
				ListElement { itemtext: "Map Packages"; itemicon: "qrc:///images/map.png" }
				ListElement { itemtext: "Bookmarks"; itemicon: "qrc:///images/oxygen/bookmarks.png" }
			}
			delegate: ListItem {
				id: listitem
				property Item contextMenu
				property bool menuOpen: contextMenu != null && contextMenu.parent === listitem
				height: menuOpen ? contextMenu.height + Theme.itemSizeSmall : Theme.itemSizeSmall
				
				Image {
					id: listitemicon
					source: itemicon
					anchors.left: parent.left
					anchors.leftMargin: Theme.paddingLarge
				}
				
				Label {
					text: itemtext
					truncationMode: TruncationMode.Fade
					anchors {
						left: listitemicon.right
						right: parent.right
						//margins: Theme.paddingLarge
						topMargin: Theme.paddingLarge
						leftMargin: Theme.paddingLarge
					}
				}

				onClicked: {
					var mainviewpage = pageStack.find(function (page) {
						return page.objectName == "mainviewpage";
					});
					if (index == 0) {
						mainviewpage.paintwidget.setDestination(mainviewpage.paintwidget.startMouseX, mainviewpage.paintwidget.startMouseY)
						pageStack.pop()
					} else if (index == 1) {
						if (!contextMenu)
							contextMenu = defaultcontextmenu.createObject(listitem)
						contextMenu.show(listitem)
					} else if (index == 2) {
						mainviewpage.mainview.setNavigation(true);
						mainviewpage.navigation = true;
						pageStack.pop()
					} else if (index == 3) {
						pageStack.push(Qt.resolvedUrl("MapPackagesPage.qml"))
					} else if (index == 4) {
						pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
					}
				}
			}
			
			Component {
				id: defaultcontextmenu
				ContextMenu {
					MenuItem {
						text: "Set to tap position"
						onClicked: {
							var mainviewpage = pageStack.find(function (page) {
								return page.objectName == "mainviewpage";
							});
							mainviewpage.paintwidget.setSource(mainviewpage.paintwidget.startMouseX, mainviewpage.paintwidget.startMouseY)
							pageStack.pop()
						}
					}
					MenuItem {
						text: "Follow your current location"
						onClicked: {
							tapmenu.setSourceFollowLocation(true);
							pageStack.pop()
						}
					}
				}
			}
		}
	}
	
	ScrollDecorator { flickable: flick }
}
