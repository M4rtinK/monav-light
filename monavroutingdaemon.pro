TEMPLATE = subdirs
SUBDIRS = routingdaemon plugins daemontest
routingdaemon.depends = plugins
plugins.file = plugins/routingdaemon_plugins.pro
daemontest.file = routingdaemon/daemontest.pro
