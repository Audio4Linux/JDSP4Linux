#ifndef TRAYICON_H
#define TRAYICON_H

#include <QAction>
#include <QSystemTrayIcon>

class TrayIcon :
	public QObject
{
	Q_OBJECT

public:
	explicit TrayIcon(QObject *parent = nullptr);

	QMenu* buildAvailableActions();
	QMenu* buildDefaultActions();

	QMenu* getTrayMenu();
	void   updateTrayMenu(QMenu *menu);
	bool   isVisible() const;

protected:
	void   iconEventHandler(QSystemTrayIcon::ActivationReason reason);

public slots:
	void   changedDisableFx(bool disabled);
	void   setTrayVisible(bool visible);

signals:
	void   iconActivated();
	void   loadPreset(const QString &path);
	void   loadIrs(const QString &path);
	void   loadReverbPreset(const QString &name);
	void   loadEqPreset(const QString &name);
	void   loadCrossfeedPreset(int name);
	void   changeDisableFx(bool disable);
	void   restart();

private:
	void   updatePresetList();
	void   createTrayIcon();
	void   updateConvolverList();

	QSystemTrayIcon *trayIcon   = nullptr;
	QAction *quitAction         = nullptr;
	QAction *tray_disableAction = nullptr;
	QMenu *tray_presetMenu      = nullptr;
	QMenu *tray_convMenu        = nullptr;
	QWidget *menuOwner          = new QWidget();
};


#endif // TRAYICON_H