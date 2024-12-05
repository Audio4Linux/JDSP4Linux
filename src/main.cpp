#ifdef USE_PULSEAUDIO
#include <PulseAudioService.h>
#else
#include <PipewireAudioService.h>
#endif

#ifndef HEADLESS
#include "MainWindow.h"
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QSessionManager>
#else
#include <QCoreApplication>
#endif

#include "config/DspConfig.h"
#include "data/AssetManager.h"
#include "data/PresetRule.h"
#include "utils/CliRemoteController.h"
#include "utils/FindBinary.h"
#include "utils/SingleInstanceMonitor.h"
#include "utils/VersionMacros.h"
#include "utils/dbus/IpcHandler.h"

#include <IAudioService.h>

#include <QCommandLineParser>
#include <QScopeGuard>
#include <QTextStream>
#include <QTranslator>

#include <sys/stat.h>
#include <sys/time.h>

#ifndef HEADLESS
static QTranslator* qtTranslator = nullptr;
static QTranslator* translator = nullptr;
#endif


#ifndef HEADLESS
void initTranslator(const QLocale& locale) {
    qtTranslator = new QTranslator(qApp);
    translator = new QTranslator(qApp);

    // Locale & translation setup
    bool _ = qtTranslator->load(locale, "qt", "_", QtCompat::libraryPath(QLibraryInfo::TranslationsPath));
    QApplication::installTranslator(qtTranslator);
    _ = translator->load(locale, "jamesdsp", "_", ":/translations");
    QApplication::installTranslator(translator);
    Q_UNUSED(_)
}
#endif

IAudioService* initAudioService() {
    // Prepare audio subsystem
    IAudioService* service;

    Log::information("============ Initializing audio service ============");
#ifdef USE_PULSEAUDIO
    Log::information("Compiled with PulseAudio support.");
    Log::information("This application flavor does not support PipeWire or its PulseAudio compatibility mode.");
    Log::information("If you want to use this application with PipeWire, you need to recompile this app with proper support enabled.");
    Log::information("Refer to the README for more detailed information.");
    Log::information("");
    service = new PulseAudioService();
#else
    Log::information("Compiled with PipeWire support.");
    Log::information("This application flavor does not support PulseAudio.");
    Log::information("If you want to use this application with PulseAudio, you need to recompile this app with proper support enabled.");
    Log::information("Refer to the README for more detailed information.");
    Log::information("");
    Log::debug("Blocklisted apps: " + AppConfig::instance().get<QString>(AppConfig::AudioAppBlocklist) /* explicitly use as QString here */);
    Log::debug("Blocklist mode: " + QString((AppConfig::instance().get<bool>(AppConfig::AudioAppBlocklistInvert) ? "allow" : "block")));
    service = new PipewireAudioService();
#endif
    QObject::connect(service, &IAudioService::logOutputReceived, [](const QString& msg){ Log::kernel(msg); });
    QObject::connect(&DspConfig::instance(), &DspConfig::updated, service, &IAudioService::update);
    QObject::connect(&DspConfig::instance(), &DspConfig::updatedExternally, service, &IAudioService::update);

    return service;
}

using namespace std;
int main(int   argc,
         char *argv[])
{
    // Locale workaround
    auto systemLocale = QLocale::system();
    QLocale::setDefault(QLocale::c());
    setlocale(LC_NUMERIC, "C");

    // Used for auto-start setup
    findyourself_init(argv[0]);
    char exepath[PATH_MAX];
    find_yourself(exepath, sizeof(exepath));
    // Ensure temp directory exists
    mkdir("/tmp/jamesdsp/", S_IRWXU);

    qRegisterMetaType<AppConfig::Key>();
    qRegisterMetaType<DspConfig::Key>();

    qDBusRegisterMetaType<PresetRule>();
    qDBusRegisterMetaType<QList<PresetRule>>();
    qDBusRegisterMetaType<Route>();
    qDBusRegisterMetaType<QList<Route>>();
    qDBusRegisterMetaType<IOutputDevice>();
    qDBusRegisterMetaType<QList<IOutputDevice>>();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif

    QScopedPointer<QCoreApplication> app(new QCoreApplication(argc, argv));
#ifndef HEADLESS
    initTranslator(systemLocale);
#endif

    QCoreApplication::setApplicationName("jamesdsp");
    QCoreApplication::setApplicationVersion(APP_VERSION_FULL);

    QCommandLineOption help(QStringList() << "h" << "help", "Displays help on command line options");
    QCommandLineOption tray(QStringList() << "t" << "tray", "Start minimized in systray (GUI)");
    QCommandLineOption watch(QStringList() << "w" << "watch", "Watch audio.conf and apply changes made by external apps automatically (GUI)");
    QCommandLineOption lang(QStringList() << "l" << "lang", "Override language (example: de, es, uk, zh_CN)", "lang");
    QCommandLineOption silent(QStringList() << "s" << "silent", "Suppress log output");
    QCommandLineOption minVerbosity(QStringList() << "m" << "min-verbosity", "Minimum log verbosity (0 = Debug; ...; 5 = Critical)", "level");
    QCommandLineOption nocolor(QStringList() << "c" << "no-color", "Disable colored log output");


    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("JamesDSP is an advanced audio processing engine available for Linux and Android systems."));
    parser.addHelpOption();
    parser.addVersionOption();

    // GUI
    parser.addOptions({tray, watch, lang});

    // Logging
    parser.addOptions({silent, nocolor, minVerbosity});

    // Remote control
    auto ctrl = CliRemoteController();
    ctrl.registerOptions(parser);

    parser.process(*app.get());

    // Determine whether CLI mode should be activated
    bool cliMode = ctrl.isAnyArgumentSet(parser);

    // Prepare logger
    bool minVerbValid = false;
    int minVerbValue = parser.value(minVerbosity).toInt(&minVerbValid);
    Log::instance().setMinSeverity(minVerbValid ? static_cast<Log::Severity>(minVerbValue) : (cliMode ? Log::Info : Log::Debug));
    Log::instance().setSilent(parser.isSet(silent));
    Log::instance().setColoredOutput(!parser.isSet(nocolor));
    Log::instance().setLoggingMode(cliMode ? Log::LM_STDOUT : Log::LM_ALL);
    Log::instance().setUseSimpleFormat(cliMode);

    if(cliMode) {
        // Enable dbus client
        ctrl.attachClient();

        // Parse CLI options
        // Set exit code accordingly
        return ctrl.execute(parser) ? 0 : 1;
    }
    else {
        AppConfig::instance().set(AppConfig::ExecutablePath, QString::fromLocal8Bit(exepath));
        Log::clear();

#ifndef HEADLESS
        // Restart qApp in GUI service mode
        app.reset();
        app.reset(new QApplication(argc, argv));
#endif

        // Check if another instance is already running and switch to it if that's the case
        QScopedPointer instanceMonitor(new SingleInstanceMonitor());
        if(!instanceMonitor->isServiceReady()) {
#ifdef HEADLESS
            Log::console("Another JamesDSP instance is already active in the background. For information about controlling the active instance, see '--help'.", true);
#else
            // Only push existing app to foreground if not launched using --tray
            if(!parser.isSet(tray))
                instanceMonitor->handover();
#endif
            return 1;
        }

        Log::information("Application version: " + QString(APP_VERSION_FULL));
        Log::information("Qt library version: " + QString(qVersion()));

        QScopedPointer audioService(initAudioService());

        // Extract default EEL files if missing
        {
            if (AppConfig::instance().get<bool>(AppConfig::LiveprogAutoExtract))
            {
                AssetManager::instance().extractAll();
            }
        }

        DspConfig::instance(parser.isSet(watch)).load();
        QScopedPointer ipc(new IpcHandler(audioService.get(), qApp));

#ifndef HEADLESS
        // Setup GUI
        QString langOverride = parser.value(lang);
        initTranslator(langOverride.isEmpty() ? systemLocale : QLocale(langOverride));

        Log::information("Using language: " + QString(QLocale::system().name()));
        Log::debug("Launched by system session manager: " + QString(qApp->isSessionRestored() ? "yes" : "no")); /* unreliable */

        // Workaround: Block DE's to resurrect multiple instances
        QObject::connect(qApp, &QGuiApplication::saveStateRequest, qApp, [](QSessionManager &manager){
                // Block session restore requests
                manager.setRestartHint(QSessionManager::RestartNever);
                manager.release();
            }, Qt::DirectConnection);

        bool launchInTray = parser.isSet(tray);

        auto* window = new MainWindow(audioService.get(), launchInTray);
        QObject::connect(instanceMonitor.get(), &SingleInstanceMonitor::raiseWindow, window, &MainWindow::raiseWindow);

        window->setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                window->size(),
                QGuiApplication::primaryScreen()->geometry()
                )
            );
        window->setWindowFlags(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);
        window->hide();

        if (!launchInTray)
        {
            /* Session manager: Prevent system from launching this app maximized on session restore (= system startup).
         * Affects DEs with enabled session restore feature; is independent from the built-in autostart feature */
            if(!qApp->isSessionRestored())
            {
                window->show();
            }
        }
#endif
        return QCoreApplication::exec();
    }
}
