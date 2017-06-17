#include "application.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "emojis.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "logging/loggingmanager.hpp"
#include "resources.hpp"
#include "settingsmanager.hpp"
#include "widgets/mainwindow.hpp"
#include "windowmanager.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QStandardPaths>
#include <boost/signals2.hpp>
#include <pajlada/settings/settingmanager.hpp>

namespace {

inline bool initSettings(bool portable)
{
    QString settingsPath;
    if (portable) {
        settingsPath.append(QDir::currentPath());
    } else {
        // Get settings path
        settingsPath.append(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        if (settingsPath.isEmpty()) {
            printf("Error finding writable location for settings\n");
            return false;
        }
    }

    if (!QDir().mkpath(settingsPath)) {
        printf("Error creating directories for settings: %s\n", qPrintable(settingsPath));
        return false;
    }
    settingsPath.append("/settings.json");

    pajlada::Settings::SettingManager::load(qPrintable(settingsPath));

    return true;
}

}  // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setAttribute(Qt::AA_EnableHighDpiScaling, true);

    // Options
    bool portable = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "portable") == 0) {
            portable = true;
        }
    }

    // Initialize settings
    if (!initSettings(portable)) {
        printf("Error initializing settings\n");
        return 1;
    }

    chatterino::logging::init();
    chatterino::SettingsManager::getInstance().load();
    chatterino::Emojis::loadEmojis();

    int ret = 0;

    {
        // Initialize application
        chatterino::Application app;

        // Start the application
        ret = app.run(a);

        // Application will go out of scope here and deinitialize itself
    }

    chatterino::SettingsManager::getInstance().save();

    // Save settings
    pajlada::Settings::SettingManager::save();

    return ret;
}
