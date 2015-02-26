TEMPLATE = subdirs
SUBDIRS = routingdaemon plugins daemontest routingserver
routingdaemon.depends = plugins
plugins.file = plugins/routingdaemon_plugins.pro
daemontest.file = routingdaemon/daemontest.pro
routingserver.file = routingdaemon/routingserver.pro
