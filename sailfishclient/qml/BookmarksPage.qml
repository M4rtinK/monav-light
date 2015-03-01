import QtQuick 2.0
import harbour.monav 1.0
import Sailfish.Silica 1.0

Page {
	id: bookmarkspage
	objectName: "bookmarkspage"
	property alias bookmarks : bookmarks
	
	Bookmarks {
		id: bookmarks
		objectName: "bookmarks"
	}

	SilicaFlickable {
		anchors.fill: parent
		
		PullDownMenu {
			MenuItem {
				text: "Add destination as a bookmark"
				onClicked: {
					pageStack.push(adddialog);
				}
			}
		}
		
		Column {
			anchors.left: parent.left
			anchors.right: parent.right
			
			PageHeader {
				anchors.left: parent.left
				anchors.right: parent.right
				title: "Bookmarks"
			}
		
			SilicaListView {
				anchors.left: parent.left
				anchors.right: parent.right
				height: childrenRect.height
				property int selindex
				id: view
				model: ListModel {
					id: bookmarkslist
				}
				delegate: ListItem {
					id: listitem
					property Item contextMenu
					property bool menuOpen: contextMenu != null && contextMenu.parent === listitem
					height: menuOpen ? contextMenu.height + Theme.itemSizeSmall : Theme.itemSizeSmall
					Label {
						text: itemtext
						truncationMode: TruncationMode.Fade
						anchors {
							left: parent.left
							right: parent.right
							margins: Theme.paddingLarge
						}
					}

					onClicked: {
						view.selindex = index
						if (!contextMenu)
							contextMenu = defaultcontextmenu.createObject(listitem)
						contextMenu.show(listitem)
					}
				}
				
				Component {
					id: defaultcontextmenu
					ContextMenu {
						MenuItem {
							text: "Set as a destination"
							onClicked: {
								console.log(view.selindex);
								bookmarks.setBookmark(view.selindex);
								var mainview = pageStack.find(function (page) {
									return page.objectName == "mainviewpage";
								});
								pageStack.pop(mainview);
							}
						}
						MenuItem {
							text: "Delete"
							onClicked: {
								bookmarks.delBookmark(view.selindex);
								view.update(bookmarks.bookmarks);
							}
						}
					}
				}
				
				function update(list) {
					bookmarkslist.clear();
					
					for (var i = 0; i < list.length; i++) {
						bookmarkslist.append({itemtext: list[i]});
					}
				}
			}
		}
	}

	Component {
		id: adddialog
		Dialog {
			Column {
				anchors.fill: parent
				
				DialogHeader {
					acceptText: "Add"
				}
				
				TextField {
					id: bookmarkname
					anchors.left: parent.left
					anchors.right: parent.right
					placeholderText: "Name for bookmark"
				}
			}
			
			onDone: {
				if (result == DialogResult.Accepted) {
					bookmarks.addBookmark(bookmarkname.text);
				}
			}
		}
	}
	
	onStatusChanged: {
		if (status == PageStatus.Activating) {
			view.update(bookmarks.bookmarks);
		}
	}
}

