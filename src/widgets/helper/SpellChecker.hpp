// src/widgets/helper/SpellChecker.hpp
#pragma once

#include <QObject>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include <QMutex>

#ifdef Q_OS_WIN
#include <windows.h>
#include <spellcheck.h>
#elif defined(Q_OS_MACOS)
#include <AppKit/AppKit.h>
#elif defined(Q_OS_LINUX)
#include <hunspell/hunspell.hxx>
#endif

namespace chatterino {

class SpellChecker : public QObject
{
    Q_OBJECT

public:
    explicit SpellChecker(QObject *parent = nullptr);
    ~SpellChecker();

    bool isEnabled() const;
    void setEnabled(bool enabled);
    
    bool isWordCorrect(const QString &word);
    QStringList suggestions(const QString &word);
    void addToPersonalDictionary(const QString &word);
    
    QString currentLanguage() const;
    void setLanguage(const QString &language);
    QStringList availableLanguages() const;

signals:
    void enabledChanged(bool enabled);
    void languageChanged(const QString &language);

private:
    void initializePlatformSpellChecker();
    void cleanup();
    
    bool m_enabled;
    QString m_currentLanguage;
    QStringList m_personalDictionary;
    QHash<QString, bool> m_wordCache;
    QMutex m_cacheMutex;
    
#ifdef Q_OS_WIN
    ISpellCheckerFactory *m_spellCheckerFactory;
    ISpellChecker *m_spellChecker;
#elif defined(Q_OS_MACOS)
    NSSpellChecker *m_nsSpellChecker;
#elif defined(Q_OS_LINUX)
    Hunspell *m_hunspell;
#endif
};

} // namespace chatterino
