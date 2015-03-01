import QtQuick 2.0
import harbour.monav 1.0
import Sailfish.Silica 1.0

Page {
	id: mappackagespage
	objectName: "mappackagespage"
	property alias mappackages: mappackages
	property alias packageselect: packageselect
	property alias routingselect: routingselect
	property alias renderingselect: renderingselect
	property alias addresslookupselect: addresslookupselect
	
	MapPackages {
		id: mappackages
		objectName: "mappackages"

		onMapPackagesLoaded: {
			mappackagespage.packageselect.update(list, selected);
			mappackagespage.routingselect.update(routinglist);
			mappackagespage.renderingselect.update(renderinglist);
			mappackagespage.addresslookupselect.update(addresslookuplist);
		}
	}
	
	WorldMapChooser {
		id: worldmapchooser
		objectName: "worldmapchooser"
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
			bottom: packageselect.top
		}
	}
	
	ComboBox {
		id: packageselect
		width: parent.width
		label: "Map package"
		property int selected
		
		anchors {
			left: parent.left
			right: parent.right
			bottom: routingselect.top
		}

		menu: ContextMenu {
			Repeater {
				model: ListModel {
					id: packageselectmodel
				}
				MenuItem { text: itemtext }
			}
			
			onActivated: {
				mappackages.changeMapPackage(index);
			}
		}
		
        function update(list, selected) {
			packageselectmodel.clear();

			for (var i = 0; i < list.length; i++) {
				packageselectmodel.append({itemtext: list[i]});
			}
			
			packageselect.selected = selected
			
			packageselecttimer.restart();
        }
        
		Timer {
			id: packageselecttimer
			interval: 500
			onTriggered: {
				packageselect.currentIndex = packageselect.selected;
			}
		}
	}

	ComboBox {
		id: routingselect
		width: parent.width
		label: "Routing module"
		
		anchors {
			left: parent.left
			right: parent.right
			bottom: renderingselect.top
		}

		menu: ContextMenu {
			Repeater {
				model: ListModel {
					id: routingselectmodel
				}
				MenuItem { text: itemtext }
			}
		}
		
        function update(list) {
			routingselectmodel.clear();

			for (var i = 0; i < list.length; i++) {
				routingselectmodel.append({itemtext: list[i]});
			}
        }
	}
	
	ComboBox {
		id: renderingselect
		width: parent.width
		label: "Rendering module"
		
		anchors {
			left: parent.left
			right: parent.right
			bottom: addresslookupselect.top
		}

		menu: ContextMenu {
			Repeater {
				model: ListModel {
					id: renderingselectmodel
				}
				MenuItem { text: itemtext }
			}
		}
		
        function update(list) {
			renderingselectmodel.clear();

			for (var i = 0; i < list.length; i++) {
				renderingselectmodel.append({itemtext: list[i]});
			}
        }
	}
	
	ComboBox {
		id: addresslookupselect
		width: parent.width
		label: "Address lookup module"
		
		anchors {
			left: parent.left
			right: parent.right
			bottom: parent.bottom
		}

		menu: ContextMenu {
			Repeater {
				model: ListModel {
					id: addresslookupselectmodel
				}
				MenuItem { text: itemtext }
			}
		}
		
        function update(list) {
			addresslookupselectmodel.clear();

			for (var i = 0; i < list.length; i++) {
				addresslookupselectmodel.append({itemtext: list[i]});
			}
        }
	}
	
	onStatusChanged: {
		if (status == PageStatus.Activating) {
			mappackages.loadMapPackages();
			timer.restart();
		} else if (status == PageStatus.Deactivating) {
			console.log(renderingselect.index);
			mappackages.loadMapModules(routingselect.currentIndex, renderingselect.currentIndex, addresslookupselect.currentIndex);
			
			var mainviewpage = pageStack.find(function (page) {
				return page.objectName == "mainviewpage";
			});
			mainviewpage.mainview.checkMapData();
		}
	}
	
	Timer {
		id: timer
		interval: 500
		onTriggered: {
			var modulenames = mappackages.modulenames;
			for (var i = 0; i < routingselectmodel.count; i++) {
				if (routingselectmodel.get(i).itemtext == modulenames[0]) {
					routingselect.currentIndex = i;
				}
			}
			for (var i = 0; i < renderingselectmodel.count; i++) {
				if (renderingselectmodel.get(i).itemtext == modulenames[1]) {
					renderingselect.currentIndex = i;
				}
			}
			for (var i = 0; i < addresslookupselectmodel.count; i++) {
				if (addresslookupselectmodel.get(i).itemtext == modulenames[2]) {
					addresslookupselect.currentIndex = i;
				}
			}
		}
	}
}


