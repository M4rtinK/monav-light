DEFINES += NOGUI
TEMPLATE = subdirs
SUBDIRS = monav-light plugins
monav-light.depends = plugins
plugins.file = plugins/routingdaemon_plugins.pro
