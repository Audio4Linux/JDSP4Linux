TEMPLATE = subdirs

SUBDIRS += \
    libjamesdsp \
    src

src.depends = libjamesdsp

TRANSLATIONS += translations/jamesdsp_ar.ts \
                translations/jamesdsp_cs.ts \
                translations/jamesdsp_de.ts \
                translations/jamesdsp_el.ts \
                translations/jamesdsp_en.ts \
                translations/jamesdsp_eo.ts \
                translations/jamesdsp_fr.ts \
                translations/jamesdsp_it.ts \
                translations/jamesdsp_jp.ts \
                translations/jamesdsp_ko.ts \
                translations/jamesdsp_no.ts \
                translations/jamesdsp_ru.ts \
                translations/jamesdsp_sv.ts \
                translations/jamesdsp_zh.ts

system(lrelease \"$$_PRO_FILE_\")

tr.commands = lupdate \"$$_PRO_FILE_\" && lrelease \"$$_PRO_FILE_\"
    PRE_TARGETDEPS += tr
    QMAKE_EXTRA_TARGETS += tr
