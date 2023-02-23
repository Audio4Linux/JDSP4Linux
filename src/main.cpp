#include "MainWindow.h"
#include "config/DspConfig.h"
#include "utils/FindBinary.h"
#include "utils/SingleInstanceMonitor.h"
#include "utils/VersionMacros.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QScreen>
#include <QStyle>
#include <QScopeGuard>
#include <QTextStream>
#include <QSessionManager>
#include <QTranslator>

#include <sys/stat.h>
#include <sys/time.h>

#ifndef NO_CRASH_HANDLER
#include "crash/airbag.h"
#include "crash/stacktrace.h"

static bool SPIN_ON_CRASH = false;

void onExceptionRaised(int fd)
{
    Q_UNUSED(fd)
	safe_printf(STDERR_FILENO, "Done! Crash report saved to /tmp/jamesdsp/crash.dmp.\n");

    if(SPIN_ON_CRASH)
    {
        safe_printf(STDERR_FILENO, "\nSpinning. Please run 'gdb jamesdsp %u' to continue debugging, Ctrl-C to quit, or Ctrl-\\ to dump core\n", getpid());
        while(true)
        {
            sleep(1);
        }
    }
    else
    {
        safe_printf(STDERR_FILENO, "\nConsider to launch this application with the parameter '--spinlock-on-crash' to wait for a debugger in case of a crash.\n");
    }
}

#endif

using namespace std;
int main(int   argc,
         char *argv[])
{
	findyourself_init(argv[0]);
	char exepath[PATH_MAX];
	find_yourself(exepath, sizeof(exepath));

    mkdir("/tmp/jamesdsp/", S_IRWXU);

#ifndef NO_CRASH_HANDLER
    QFile crashDmp(STACKTRACE_LOG);

    bool lastSessionCrashed = false;
    if(crashDmp.exists() && crashDmp.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&crashDmp);
        auto lastDmp = in.readAll();
        crashDmp.close();

        if(lastDmp.length() > 10)
        {
            lastSessionCrashed = true;
            crashDmp.copy(STACKTRACE_LOG_OLD);
            Log::backupLastLog();
        }
    }

	EXECUTION_FILENAME = exepath;
	int                fd = safe_open_wo_fd("/tmp/jamesdsp/crash.dmp");
    airbag_init_fd(fd, onExceptionRaised, EXECUTION_FILENAME);
#endif

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QApplication app(argc, argv);

	QCommandLineParser parser;
	parser.setApplicationDescription("JamesDSP for Linux");
	parser.addHelpOption();
    QCommandLineOption tray(QStringList() << "t" << "tray", "Start minimized in systray (forced)");
    parser.addOption(tray);
    QCommandLineOption watch(QStringList() << "w" << "watch", "Watch audio.conf and apply changes made by external apps automatically");
    parser.addOption(watch);
    QCommandLineOption minst(QStringList() << "m" << "allow-multiple-instances", "Allow multiple instances of this app");
    parser.addOption(minst);
    QCommandLineOption spinlck(QStringList() << "d" << "spinlock-on-crash", "Wait for debugger in case of crash");
    parser.addOption(spinlck);
    QCommandLineOption silent(QStringList() << "s" << "silent", "Suppress log output");
    parser.addOption(silent);
    QCommandLineOption nocolor(QStringList() << "c" << "no-color", "Disable colored log output");
    parser.addOption(nocolor);
    QCommandLineOption lang(QStringList() << "l" << "lang", "Force language (two letter country code)", "lang");
    parser.addOption(lang);
    parser.process(app);

    QString defaultLocale = QLocale::system().name(); // e.g. "de_DE"
    defaultLocale.truncate(defaultLocale.lastIndexOf('_')); // e.g. "de"
    QString locale = (parser.isSet(lang) ? parser.value(lang) : defaultLocale);

    QTranslator translator;
    translator.load(":/translations/jamesdsp_" + locale + ".qm");
    app.installTranslator(&translator);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + locale + ".qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

#ifndef NO_CRASH_HANDLER
    SPIN_ON_CRASH = parser.isSet(spinlck);
#endif

    // Locale workaround
    QLocale::setDefault(QLocale::c());
    if(setlocale(LC_NUMERIC, "en_US.UTF-8") == nullptr)
    {
        setlocale(LC_NUMERIC, "C");
    }

    // Prepare logger
    Log::instance().setSilent(parser.isSet(silent));
    Log::instance().setColoredOutput(!parser.isSet(nocolor));

    Log::clear();

    Log::information("Application version: " + QString(APP_VERSION_FULL));
    Log::information("Qt library version: " + QString(qVersion()));
    Log::information("Using language: " + QString(locale));

    Log::debug("Launched by system session manager: " + QString(qApp->isSessionRestored() ? "yes" : "no")); /* unreliable */
    QGuiApplication::setFallbackSessionManagementEnabled(false);   
    QObject::connect(qApp, &QGuiApplication::saveStateRequest, qApp, [](QSessionManager &manager){
        // Block session restore requests
        manager.setRestartHint(QSessionManager::RestartNever);
        manager.release();
    }, Qt::DirectConnection);

    // Check if another instance is already running and switch to it if that's the case
    auto *instanceMonitor = new SingleInstanceMonitor();
    auto scopeGuard = qScopeGuard([instanceMonitor]{ delete instanceMonitor; });
    if (!instanceMonitor->isServiceReady() && !parser.isSet(minst))
    {
        instanceMonitor->handover();
        return 0;
    }

#ifndef NO_CRASH_HANDLER
    if(lastSessionCrashed)
    {
        Log::information("Last session crashed unexpectedly. A crash report has been saved here: " + QString(STACKTRACE_LOG_OLD));
    }
#endif

    // Prepare DspConfig based on cmdline argument
    DspConfig::instance(parser.isSet(watch));
    AppConfig::instance().set(AppConfig::ExecutablePath, QString::fromLocal8Bit(exepath));

    MainWindow w(parser.isSet(tray));

    QObject::connect(instanceMonitor, &SingleInstanceMonitor::raiseWindow, &w, &MainWindow::raiseWindow);

	w.setGeometry(
		QStyle::alignedRect(
			Qt::LeftToRight,
			Qt::AlignCenter,
			w.size(),
			QGuiApplication::primaryScreen()->geometry()
			)
		);
	w.setWindowFlags(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);
    w.hide();

    if (!parser.isSet(tray))
	{
        /* Session manager: Prevent system from launching this app maximized on session restore (= system startup).
         * Affects DEs with enabled session restore feature; is independent from the built-in autostart feature */
        if(!qApp->isSessionRestored())
        {
            w.show();
        }
    }

	return QApplication::exec();
}
