TEMPLATE = subdirs

SUBDIRS += \
    docking-system \
    src
	
docking-system.subdir = 3rdparty/docking-system/src

src.depends = docking-system
