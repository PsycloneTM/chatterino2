
#include "singletons/Theme.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSet>

#include <cmath>

namespace {

using namespace chatterino;

void parseInto(const QJsonObject &obj, const QLatin1String &key, QColor &color)
{
    const auto &jsonValue = obj[key];
    if (!jsonValue.isString()) [[unlikely]]
    {
        qCWarning(chatterinoTheme) << key
                                   << "was expected but not found in the "
                                      "current theme - using previous value.";
        return;
    }
    QColor parsed = {jsonValue.toString()};
    if (!parsed.isValid()) [[unlikely]]
    {
        qCWarning(chatterinoTheme).nospace()
            << "While parsing " << key << ": '" << jsonValue.toString()
            << "' isn't a valid color.";
        return;
    }
    color = parsed;
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define parseColor(to, from, key) \
    parseInto(from, QLatin1String(#key), (to).from.key)
// NOLINTEND(cppcoreguidelines-macro-usage)

void parseWindow(const QJsonObject &window, chatterino::Theme &theme)
{
    parseColor(theme, window, background);
    parseColor(theme, window, text);
}

void parseTabs(const QJsonObject &tabs, chatterino::Theme &theme)
{
    const auto parseTabColors = [](auto json, auto &tab) {
        parseInto(json, QLatin1String("text"), tab.text);
        {
            const auto backgrounds = json["backgrounds"].toObject();
            parseColor(tab, backgrounds, regular);
            parseColor(tab, backgrounds, hover);
            parseColor(tab, backgrounds, unfocused);
        }
        {
            const auto line = json["line"].toObject();
            parseColor(tab, line, regular);
            parseColor(tab, line, hover);
            parseColor(tab, line, unfocused);
        }
    };
    parseColor(theme, tabs, dividerLine);
    parseTabColors(tabs["regular"].toObject(), theme.tabs.regular);
    parseTabColors(tabs["newMessage"].toObject(), theme.tabs.newMessage);
    parseTabColors(tabs["highlighted"].toObject(), theme.tabs.highlighted);
    parseTabColors(tabs["selected"].toObject(), theme.tabs.selected);
}

void parseMessages(const QJsonObject &messages, chatterino::Theme &theme)
{
    {
        const auto textColors = messages["textColors"].toObject();
        parseColor(theme.messages, textColors, regular);
        parseColor(theme.messages, textColors, caret);
        parseColor(theme.messages, textColors, link);
        parseColor(theme.messages, textColors, system);
        parseColor(theme.messages, textColors, chatPlaceholder);
    }
    {
        const auto backgrounds = messages["backgrounds"].toObject();
        parseColor(theme.messages, backgrounds, regular);
        parseColor(theme.messages, backgrounds, alternate);
    }
    parseColor(theme, messages, disabled);
    parseColor(theme, messages, selection);
    parseColor(theme, messages, highlightAnimationStart);
    parseColor(theme, messages, highlightAnimationEnd);
}

void parseScrollbars(const QJsonObject &scrollbars, chatterino::Theme &theme)
{
    parseColor(theme, scrollbars, background);
    parseColor(theme, scrollbars, thumb);
    parseColor(theme, scrollbars, thumbSelected);
}

void parseSplits(const QJsonObject &splits, chatterino::Theme &theme)
{
    parseColor(theme, splits, messageSeperator);
    parseColor(theme, splits, background);
    parseColor(theme, splits, dropPreview);
    parseColor(theme, splits, dropPreviewBorder);
    parseColor(theme, splits, dropTargetRect);
    parseColor(theme, splits, dropTargetRectBorder);
    parseColor(theme, splits, resizeHandle);
    parseColor(theme, splits, resizeHandleBackground);

    {
        const auto header = splits["header"].toObject();
        parseColor(theme.splits, header, border);
        parseColor(theme.splits, header, focusedBorder);
        parseColor(theme.splits, header, background);
        parseColor(theme.splits, header, focusedBackground);
        parseColor(theme.splits, header, text);
        parseColor(theme.splits, header, focusedText);
    }
    {
        const auto input = splits["input"].toObject();
        parseColor(theme.splits, input, background);
        parseColor(theme.splits, input, text);
    }
}

void parseColors(const QJsonObject &root, chatterino::Theme &theme)
{
    const auto colors = root["colors"].toObject();

    parseInto(colors, QLatin1String("accent"), theme.accent);

    parseWindow(colors["window"].toObject(), theme);
    parseTabs(colors["tabs"].toObject(), theme);
    parseMessages(colors["messages"].toObject(), theme);
    parseScrollbars(colors["scrollbars"].toObject(), theme);
    parseSplits(colors["splits"].toObject(), theme);
}
#undef parseColor

/**
 * Load the given theme descriptor from its path
 *
 * Returns a JSON object containing theme data if the theme is valid, otherwise nullopt
 *
 * NOTE: No theme validation is done by this function
 **/
std::optional<QJsonObject> loadTheme(const ThemeDescriptor &theme)
{
    QFile file(theme.path);
    if (!file.open(QFile::ReadOnly))
    {
        qCWarning(chatterinoTheme)
            << "Failed to open" << file.fileName() << "at" << theme.path;
        return std::nullopt;
    }

    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(file.readAll(), &error);
    if (!json.isObject())
    {
        qCWarning(chatterinoTheme) << "Failed to parse" << file.fileName()
                                   << "error:" << error.errorString();
        return std::nullopt;
    }

    // TODO: Validate JSON schema?

    return json.object();
}

}  // namespace

namespace chatterino {

const std::map<QString, ThemeDescriptor> Theme::builtInThemes{
    {"White", {":/themes/White.json", false}},
    {"Light", {":/themes/Light.json", false}},
    {"Dark", {":/themes/Dark.json", false}},
    {"Black", {":/themes/Black.json", false}},
};

const ThemeDescriptor Theme::fallbackTheme = Theme::builtInThemes.at("Dark");

bool Theme::isLightTheme() const
{
    return this->isLight_;
}

void Theme::initialize(Settings &settings, Paths &paths)
{
    this->themeName.connect(
        [this](auto themeName) {
            qCDebug(chatterinoTheme) << "Theme updated to" << themeName;
            this->update();
        },
        false);

    this->loadAvailableThemes();

    this->update();
}

void Theme::update()
{
    auto theme = this->availableThemes_.find(this->themeName);

    std::optional<QJsonObject> themeJSON;

    if (theme == this->availableThemes_.end())
    {
        qCWarning(chatterinoTheme)
            << "Theme" << this->themeName
            << "not found, falling back to the fallback theme";

        themeJSON = loadTheme(fallbackTheme);
    }
    else
    {
        themeJSON = loadTheme(theme->second);

        if (!themeJSON)
        {
            qCWarning(chatterinoTheme)
                << "Theme" << this->themeName
                << "not valid, falling back to the fallback theme";

            // Parsing the theme failed, fall back
            themeJSON = loadTheme(fallbackTheme);
        }
    }

    if (!themeJSON)
    {
        qCWarning(chatterinoTheme)
            << "Failed to load" << this->themeName << "or the fallback theme";
        return;
    }

    this->parseFrom(*themeJSON);

    this->updated.invoke();
}

QStringList Theme::availableThemeNames() const
{
    QStringList themeNames;

    for (const auto &[name, theme] : this->availableThemes_)
    {
        // NOTE: This check could also use `Theme::builtInThemes.contains(name)` instead
        if (theme.custom)
        {
            themeNames.append(QString("Custom: %1").arg(name));
        }
        else
        {
            themeNames.append(name);
        }
    }

    return themeNames;
}

void Theme::loadAvailableThemes()
{
    this->availableThemes_ = Theme::builtInThemes;

    auto dir = QDir(getPaths()->themesDirectory);
    for (const auto &info :
         dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
    {
        if (!info.isFile())
        {
            continue;
        }

        if (!info.fileName().endsWith(".json"))
        {
            continue;
        }

        auto themeDescriptor = ThemeDescriptor{info.absoluteFilePath(), true};

        auto theme = loadTheme(themeDescriptor);
        if (!theme)
        {
            qCWarning(chatterinoTheme) << "Failed to parse theme at" << info;
            continue;
        }

        auto themeName = QString("Custom: %1").arg(info.baseName());

        this->availableThemes_.emplace(themeName, themeDescriptor);
    }
}

void Theme::parseFrom(const QJsonObject &root)
{
    parseColors(root, *this);

    this->isLight_ =
        root["metadata"]["iconTheme"].toString() == QStringLiteral("dark");

    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.name() + ";" +
        "color:" + this->messages.textColors.regular.name() + ";" +
        "selection-background-color:" +
        (this->isLightTheme() ? "#68B1FF"
                              : this->tabs.selected.backgrounds.regular.name());

    this->window.contextMenuStyleSheet =
        QStringLiteral("QMenu { background: %1; border: %2; color: %3; "
                       "selection-background-color: %2; } "
                       "QMenu::item:disabled { color: #8c7f7f; }")
            .arg(splits.input.background.name(QColor::HexArgb),
                 tabs.selected.backgrounds.regular.name(QColor::HexArgb),
                 tabs.selected.text.name(QColor::HexArgb));

    // Usercard buttons
    if (this->isLightTheme())
    {
        this->buttons.copy = getResources().buttons.copyDark;
        this->buttons.pin = getResources().buttons.pinDisabledDark;
    }
    else
    {
        this->buttons.copy = getResources().buttons.copyLight;
        this->buttons.pin = getResources().buttons.pinDisabledLight;
    }
}

void Theme::normalizeColor(QColor &color) const
{
    if (this->isLightTheme())
    {
        if (color.lightnessF() > 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() > 0.4 && color.hueF() > 0.1 &&
            color.hueF() < 0.33333)
        {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() - sin((color.hueF() - 0.1) /
                                                   (0.3333 - 0.1) * 3.14159) *
                                                   color.saturationF() * 0.4);
        }
    }
    else
    {
        if (color.lightnessF() < 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() < 0.6 && color.hueF() > 0.54444 &&
            color.hueF() < 0.83333)
        {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() + sin((color.hueF() - 0.54444) /
                                         (0.8333 - 0.54444) * 3.14159) *
                                         color.saturationF() * 0.4);
        }
    }
}

Theme *getTheme()
{
    return getApp()->themes;
}

}  // namespace chatterino
