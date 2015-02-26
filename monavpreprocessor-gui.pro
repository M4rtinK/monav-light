TEMPLATE = subdirs
SUBDIRS = preprocessor plugins
preprocessor.file = preprocessor/preprocessor-gui.pro
preprocessor.depends = plugins
plugins.file = plugins/preprocessor_plugins.pro
