#ifndef CLIREMOTECONTROLLER_H
#define CLIREMOTECONTROLLER_H

#include <QObject>

#include "utils/dbus/ClientProxy.h"

class CliRemoteController : public QObject
{
    Q_OBJECT
public:
    explicit CliRemoteController(QObject *parent = nullptr);
    ~CliRemoteController();

    void attachClient();
    void registerOptions(QCommandLineParser& parser) const;
    bool isAnyArgumentSet(QCommandLineParser& parser) const;
    bool execute(QCommandLineParser& parser);

private:
    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE bool set(const QString& keyValue) const;
    Q_INVOKABLE bool get(const QString& key) const;
    Q_INVOKABLE bool getAll() const;
    Q_INVOKABLE bool listKeys() const;
    Q_INVOKABLE bool listPresets() const;
    Q_INVOKABLE bool loadPreset(const QString& name) const;
    Q_INVOKABLE bool savePreset(const QString& name) const;
    Q_INVOKABLE bool deletePreset(const QString& name) const;
    Q_INVOKABLE bool listPresetRules() const;
    Q_INVOKABLE bool addPresetRule(const QString& keyValue) const;
    Q_INVOKABLE bool deletePresetRule(const QString& keyValue) const;
    Q_INVOKABLE bool listOutputDevices() const;
    Q_INVOKABLE bool showStatus() const;

private:
    me::timschneeberger::jdsp4linux::Service* service = nullptr;

    bool checkConnectionAndLog() const;
    bool requireConnectionAndLog() const;

    QList<QCommandLineOption*> _options;

#define ARGS(...) __VA_ARGS__
#define OPT(name, data) QCommandLineOption* name = new QCommandLineOption(QStringList() << data); \
    Q_PROPERTY(QCommandLineOption* name MEMBER name FINAL)

    OPT(_isConnected, ARGS("is-connected", "Check if JamesDSP service is active. Returns exit code 1 if not. (Remote)"))
    OPT(_showStatus, ARGS("status", "Show status (Remote)"))

    OPT(_listKeys, ARGS("list-keys", "List available audio configuration keys (Remote)"))
    OPT(_getAll, ARGS("get-all", "Get all audio configuration values (Remote)"))
    OPT(_get, ARGS("get", "Get audio configuration value (Remote)", "key"))
    OPT(_set, ARGS("set", "Set audio configuration value (format: key=value) (Remote)", "key=value"))

    OPT(_listPresets, ARGS("list-presets", "List presets (Remote)"))
    OPT(_loadPreset, ARGS("load-preset", "Load preset by name (Remote)", "name"))
    OPT(_savePreset, ARGS("save-preset", "Save current settings as preset (Remote)", "name"))
    OPT(_deletePreset, ARGS("delete-preset", "Delete preset by name (Remote)", "name"))

    OPT(_listOutputDevices, ARGS("list-devices", "List audio devices (Remote)"))

    OPT(_listPresetRules, ARGS("list-preset-rules", "List preset rules (Remote)"))
    OPT(_addPresetRule, ARGS("set-preset-rule", "Add/modify preset rule (Remote)", "deviceId=presetName"))
    OPT(_deletePresetRule, ARGS("delete-preset-rule", "Delete preset rule (Remote)", "deviceId"))

#undef ARGS
#undef OPT
};

Q_DECLARE_METATYPE(QCommandLineOption*)

#endif // CLIREMOTECONTROLLER_H
