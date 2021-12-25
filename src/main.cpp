#include "MainWindow.h"
#include "config/DspConfig.h"
#include "utils/FindBinary.h"
#include "utils/SingleInstanceMonitor.h"
#include "utils/VersionMacros.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QScreen>
#include <QStyle>
#include <QScopeGuard>

#define FORCE_CRASH_HANDLER

#if defined(Q_OS_UNIX) && defined(QT_NO_DEBUG) || defined(FORCE_CRASH_HANDLER)
#define ENABLE_CRASH_HANDLER
#endif

#ifdef ENABLE_CRASH_HANDLER
#include "crash/airbag.h"
#include "crash/stacktrace.h"
#include <sys/stat.h>
#include <sys/time.h>

static bool SPIN_ON_CRASH = false;
static const char* EXEC_NAME = "jamesdsp";

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

#ifdef ENABLE_CRASH_HANDLER
	EXECUTION_FILENAME = exepath;
	mkdir("/tmp/jamesdsp/", S_IRWXU);
	int                fd = safe_open_wo_fd("/tmp/jamesdsp/crash.dmp");
    airbag_init_fd(fd, onExceptionRaised, EXECUTION_FILENAME);
#endif

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
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
    parser.process(app);

#ifdef ENABLE_CRASH_HANDLER
    SPIN_ON_CRASH = parser.isSet(spinlck);
#endif

    // Locale workaround
    QLocale::setDefault(QLocale::c());
    if(setlocale(LC_NUMERIC, "en_US.UTF-8") == nullptr)
    {
        setlocale(LC_NUMERIC, "C");
    }

    // Prepare logger
    Log::clear();
    Log::information("Application version: " + QString(APP_VERSION_FULL));
    Log::debug("Launched by system session manager: " + QString(qApp->isSessionRestored() ? "yes" : "no"));

    // Check if another instance is already running and switch to it if that's the case
    auto *instanceMonitor = new SingleInstanceMonitor();
    auto scopeGuard = qScopeGuard([instanceMonitor]{ delete instanceMonitor; });
    if (!instanceMonitor->isServiceReady() && !parser.isSet(minst))
    {
        instanceMonitor->handover();
        return 0;
    }

    // Prepare DspConfig based on cmdline argument
    DspConfig::instance(parser.isSet(watch));
    AppConfig::instance().set(AppConfig::ExecutablePath, QString::fromLocal8Bit(exepath));

    QApplication::setFallbackSessionManagementEnabled(false);
    MainWindow w(parser.isSet(tray));

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
