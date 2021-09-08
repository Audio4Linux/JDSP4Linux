TEMPLATE = subdirs

SUBDIRS += \
    libjamesdsp \
    src/subprojects/EELEditor \
    src

src.depends = libjamesdsp
src.depends = src/subprojects/EELEditor
