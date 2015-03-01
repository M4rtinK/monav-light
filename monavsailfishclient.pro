TEMPLATE = subdirs
SUBDIRS = sailfishclient plugins
sailfishclient.depends = plugins
plugins.file = plugins/client_plugins.pro
