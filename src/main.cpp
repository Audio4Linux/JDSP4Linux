#include "MainWindow.h"

#include "utils/FindBinary.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QScreen>
#include <QStyle>
#include <QSystemTrayIcon>

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

void crash_handled(int fd)
{
    Q_UNUSED(fd)
	safe_printf(STDERR_FILENO, "Done! Crash report saved to /tmp/jamesdsp/crash.dmp.\n");

    if(SPIN_ON_CRASH)
    {
        safe_printf(STDERR_FILENO, "\nSpinning. Please run 'gdb jamesdsp %u' to continue debugging, Ctrl-C to quit, or Ctrl-\ to dump core\n", getpid());
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
	airbag_init_fd(fd, crash_handled, EXECUTION_FILENAME);
#endif

	QApplication       a(argc, argv);
	QCommandLineParser parser;
	parser.setApplicationDescription("JamesDSP for Linux");
	parser.addHelpOption();

	QCommandLineOption tray(QStringList() << "t" << "tray", "Start minimized in systray (forced)");
	parser.addOption(tray);
    QCommandLineOption minst(QStringList() << "m" << "allow-multiple-instances", "Allow multiple instances of this app");
    parser.addOption(minst);
    QCommandLineOption spinlck(QStringList() << "d" << "spinlock-on-crash", "Wait for debugger in case of crash");
    parser.addOption(spinlck);
	parser.process(a);

    SPIN_ON_CRASH = parser.isSet(spinlck);

	QLocale::setDefault(QLocale::c());  

	QApplication::setQuitOnLastWindowClosed(false);
	MainWindow w(QString::fromLocal8Bit(exepath), parser.isSet(tray), parser.isSet(minst));
	w.setFixedSize(w.geometry().width(), w.geometry().height());
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
		w.show();
	}

	QApplication::setQuitOnLastWindowClosed(true);
	return QApplication::exec();
}
