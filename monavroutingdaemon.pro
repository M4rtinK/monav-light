TEMPLATE = subdirs
SUBDIRS = routingdaemon plugins
routingdaemon.depends = plugins
plugins.file = plugins/routingdaemon_plugins.pro
