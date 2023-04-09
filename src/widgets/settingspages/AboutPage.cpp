#include "AboutPage.hpp"

#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTextStream>
#include <QUrl>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#include <qmarkdown.h>

#define PIXMAP_WIDTH 500

#define LINK_CHATTERINO_WIKI "https://wiki.chatterino.com"
#define LINK_DONATE "https://streamelements.com/fourtf/tip"
#define LINK_CHATTERINO_FEATURES "https://chatterino.com/#features"
#define LINK_CHATTERINO_DISCORD "https://discord.gg/7Y5AYhAK4z"

namespace chatterino {

AboutPage::AboutPage()
{
    LayoutCreator<AboutPage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();
    {
        QPixmap pixmap;
        pixmap.load(":/settings/aboutlogo.png");

        auto logo = layout.emplace<QLabel>().assign(&this->logo_);
        logo->setPixmap(pixmap);
        if (pixmap.width() != 0)
        {
            logo->setFixedSize(PIXMAP_WIDTH,
                               PIXMAP_WIDTH * pixmap.height() / pixmap.width());
        }
        logo->setScaledContents(true);

        // Version
        auto versionInfo = layout.emplace<QGroupBox>("Version");
        {
            auto vbox = versionInfo.emplace<QVBoxLayout>();
            auto version = Version::instance();

            auto label = vbox.emplace<QLabel>(version.buildString() + "<br>" +
                                              version.runningString());
            label->setOpenExternalLinks(true);
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        }

        // About Chatterino
        auto aboutChatterino = layout.emplace<QGroupBox>("About Chatterino...");
        {
            auto l = aboutChatterino.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("Chatterino Wiki can be found <a href=\"" LINK_CHATTERINO_WIKI "\">here</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("All about Chatterino's <a href=\"" LINK_CHATTERINO_FEATURES "\">features</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Join the official Chatterino <a href=\"" LINK_CHATTERINO_DISCORD "\">Discord</a>")->setOpenExternalLinks(true);
            // clang-format on
        }

        // Licenses
        auto licenses =
            layout.emplace<QGroupBox>("Open source software used...");
        {
            auto form = licenses.emplace<QFormLayout>();

            addLicense(form.getElement(), "Qt Framework", "https://www.qt.io",
                       ":/licenses/qt_lgpl-3.0.txt");
            addLicense(form.getElement(), "Boost", "https://www.boost.org/",
                       ":/licenses/boost_boost.txt");
            addLicense(form.getElement(), "LibCommuni",
                       "https://github.com/communi/libcommuni",
                       ":/licenses/libcommuni_BSD3.txt");
            addLicense(form.getElement(), "OpenSSL", "https://www.openssl.org/",
                       ":/licenses/openssl.txt");
            addLicense(form.getElement(), "RapidJson", "https://rapidjson.org/",
                       ":/licenses/rapidjson.txt");
            addLicense(form.getElement(), "Pajlada/Settings",
                       "https://github.com/pajlada/settings",
                       ":/licenses/pajlada_settings.txt");
            addLicense(form.getElement(), "Pajlada/Signals",
                       "https://github.com/pajlada/signals",
                       ":/licenses/pajlada_signals.txt");
            addLicense(form.getElement(), "Websocketpp",
                       "https://www.zaphoyd.com/websocketpp/",
                       ":/licenses/websocketpp.txt");
#ifndef NO_QTKEYCHAIN
            addLicense(form.getElement(), "QtKeychain",
                       "https://github.com/frankosterfeld/qtkeychain",
                       ":/licenses/qtkeychain.txt");
#endif
            addLicense(form.getElement(), "lrucache",
                       "https://github.com/lamerman/cpp-lru-cache",
                       ":/licenses/lrucache.txt");
            addLicense(form.getElement(), "magic_enum",
                       "https://github.com/Neargye/magic_enum",
                       ":/licenses/magic_enum.txt");
            addLicense(form.getElement(), "semver",
                       "https://github.com/Neargye/semver",
                       ":/licenses/semver.txt");
            addLicense(form.getElement(), "miniaudio",
                       "https://github.com/mackron/miniaudio",
                       ":/licenses/miniaudio.txt");
#ifdef CHATTERINO_HAVE_PLUGINS
            addLicense(form.getElement(), "lua", "https://lua.org",
                       ":/licenses/lua.txt");
            addLicense(form.getElement(), "Fluent icons",
                       "https://github.com/microsoft/fluentui-system-icons",
                       ":/licenses/fluenticons.txt");
#endif
#ifdef CHATTERINO_WITH_CRASHPAD
            addLicense(form.getElement(), "sentry-crashpad",
                       "https://github.com/getsentry/crashpad",
                       ":/licenses/crashpad.txt");
#endif
        }

        // Attributions
        auto attributions = layout.emplace<QGroupBox>("Attributions...");
        {
            auto l = attributions.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("Twemoji emojis provided by <a href=\"https://github.com/twitter/twemoji\">Twitter's Twemoji</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Facebook emojis provided by <a href=\"https://facebook.com\">Facebook</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Apple emojis provided by <a href=\"https://apple.com\">Apple</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Google emojis provided by <a href=\"https://google.com\">Google</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Emoji datasource provided by <a href=\"https://www.iamcal.com/\">Cal Henderson</a>"
                              "(<a href=\"https://github.com/iamcal/emoji-data/blob/master/LICENSE\">show license</a>)")->setOpenExternalLinks(true);
            // clang-format on
        }

// Contributors
void MainWindow::setupContributors()
{
    auto scrollArea = layout.emplace<QScrollArea>();
    auto contentWidget = scrollArea.emplace<QWidget>();
    auto layout = contentWidget.emplace<QVBoxLayout>();
    layout->setContentsMargins(0, 0, 0, 0);

    auto contributorsGroup = layout.emplace<QGroupBox>("Contributors");
    auto contributorsLayout = contributorsGroup.emplace<QVBoxLayout>();

    QFile file(":/contributors.md");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open contributors file";
        return;
    }

    QTextStream stream(&file);
    QString markdown = stream.readAll();

    // Parse the Markdown file
    QMarkdown md;
    QString html = md.render(markdown);

    // Create a QTextDocument to render the HTML as rich text
    QTextDocument doc;
    doc.setHtml(html);

    // Iterate over the document's blocks to extract the contributor information
    QTextBlock block = doc.begin();
    while (block.isValid()) {
        QTextBlockUserData *userData = block.userData();
        if (!userData) {
            // This is not a user data block, so add it to the layout as rich text
            QString text = block.text();
            if (!text.isEmpty()) {
                auto *label = new QLabel(text);
                label->setTextFormat(Qt::RichText);
                label->setTextInteractionFlags(Qt::TextBrowserInteraction);
                label->setOpenExternalLinks(true);
                contributorsLayout->addWidget(label);
            }
        } else if (qobject_cast<QMarkdownUserData*>(userData)) {
            // This is a user data block, so extract the contributor information
            auto *userData = qobject_cast<QMarkdownUserData*>(block.userData());
            const auto &contributor = userData->contributor;

            auto contributorLayout = contributorsLayout.emplace<QHBoxLayout>();
            contributorLayout->setSpacing(16);

            auto *avatarLabel = new QLabel();
            avatarLabel->setPixmap(QPixmap(contributor.avatar));
            avatarLabel->setAlignment(Qt::AlignCenter);
            contributorLayout->addWidget(avatarLabel);

            auto *infoLayout = new QVBoxLayout;
            contributorLayout->addLayout(infoLayout);

            auto *nameLabel = new QLabel(contributor.name);
            nameLabel->setFont(QFont("Helvetica", 16, QFont::Bold));
            infoLayout->addWidget(nameLabel);

            auto *roleLabel = new QLabel(contributor.role);
            infoLayout->addWidget(roleLabel);

            if (!contributor.url.isEmpty()) {
                auto *urlLabel = new QLabel(contributor.url);
                urlLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
                urlLabel->setOpenExternalLinks(true);
                infoLayout->addWidget(urlLabel);
            }
        }
        block = block.next();
    }

    file.close();
}


void AboutPage::addLicense(QFormLayout *form, const QString &name,
                           const QString &website, const QString &licenseLink)
{
    auto *a = new QLabel("<a href=\"" + website + "\">" + name + "</a>");
    a->setOpenExternalLinks(true);
    auto *b = new QLabel("<a href=\"" + licenseLink + "\">show license</a>");
    QObject::connect(
        b, &QLabel::linkActivated, [parent = this, name, licenseLink] {
            auto window = new BasePopup({BaseWindow::Flags::EnableCustomFrame,
                                         BaseWindow::DisableLayoutSave},
                                        parent);
            window->setWindowTitle("Chatterino - License for " + name);
            window->setAttribute(Qt::WA_DeleteOnClose);
            auto layout = new QVBoxLayout();
            auto *edit = new QTextEdit;

            QFile file(licenseLink);
            file.open(QIODevice::ReadOnly);
            edit->setText(file.readAll());
            edit->setReadOnly(true);

            layout->addWidget(edit);

            window->getLayoutContainer()->setLayout(layout);
            window->show();
        });

    form->addRow(a, b);
}

}  // namespace chatterino
