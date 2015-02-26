TEMPLATE = subdirs
SUBDIRS = client plugins
client.depends = plugins
plugins.file = plugins/client_plugins.pro
