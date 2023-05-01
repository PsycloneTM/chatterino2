#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/Singleton.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/settings/setting.hpp>
#include <QColor>
#include <QJsonObject>
#include <QPixmap>

#include <map>
#include <optional>

namespace chatterino {

class WindowManager;

struct ThemeDescriptor {
    // Path to the theme on disk
    // Can be a Qt resource path
    QString path;

    bool custom;
};

class Theme final : public Singleton
{
public:
    QString contextMenuStyke static const std::map<QString, ThemeDescriptor>
        builtInThemes;

    // The built in theme that will be used if some theme parsing fails
    static const ThemeDescriptor fallbackTheme;

    void initialize(Settings &settings, Paths &paths) final;

    bool isLightTheme() const;

    struct TabColors {
        QColor text;
        struct {
            QColor regular;
            QColor hover;
            QColor unfocused;
        } backgrounds;
        struct {
            QColor regular;
            QColor hover;
            QColor unfocused;
        } line;
    };

    QColor accent{"#00aeef"};

    /// WINDOW
    struct {
        QColor background;
        QColor text;
    } window;

    /// TABS
    struct {
        TabColors regular;
        TabColors newMessage;
        TabColors highlighted;
        TabColors selected;
        QColor dividerLine;
    } tabs;

    /// MESSAGES
    struct {
        struct {
            QColor regular;
            QColor caret;
            QColor link;
            QColor system;
            QColor chatPlaceholder;
        } textColors;

        struct {
            QColor regular;
            QColor alternate;
        } backgrounds;

        QColor disabled;
        QColor selection;

        QColor highlightAnimationStart;
        QColor highlightAnimationEnd;
    } messages;

    /// SCROLLBAR
    struct {
        QColor background;
        QColor thumb;
        QColor thumbSelected;
    } scrollbars;

    /// SPLITS
    struct {
        QColor messageSeperator;
        QColor background;
        QColor dropPreview;
        QColor dropPreviewBorder;
        QColor dropTargetRect;
        QColor dropTargetRectBorder;
        QColor resizeHandle;
        QColor resizeHandleBackground;

        struct {
            QColor border;
            QColor focusedBorder;
            QColor background;
            QColor focusedBackground;
            QColor text;
            QColor focusedText;
        } header;

        struct {
            QColor background;
            QColor text;
            QString styleSheet;
        } input;
    } splits;

    struct {
        QPixmap copy;
        QPixmap pin;
    } buttons;

    void normalizeColor(QColor &color) const;
    void update();

    /**
     * Return a list of available themes
     *
     * Custom themes are prefixed with "Custom: "
     **/
    QStringList availableThemeNames() const;

    pajlada::Signals::NoArgSignal updated;

    QStringSetting themeName{"/appearance/theme/name", "Dark"};

private:
    bool isLight_ = false;

    std::map<QString, ThemeDescriptor> availableThemes_;

    /**
     * Figure out which themes are available in the Themes directory
     *
     * NOTE: This is currently not built to be reloadable
     **/
    void loadAvailableThemes();

    void parseFrom(const QJsonObject &root);

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets_;

    friend class WindowManager;
};

Theme *getTheme();
}  // namespace chatterino
