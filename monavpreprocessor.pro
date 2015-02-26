TEMPLATE = subdirs
SUBDIRS = preprocessor plugins
preprocessor.depends = plugins
plugins.file = plugins/preprocessor_plugins.pro