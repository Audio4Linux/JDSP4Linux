TEMPLATE = subdirs

SUBDIRS += \
    libjamesdsp \
    src

src.depends = libjamesdsp
