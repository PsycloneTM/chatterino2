// src/widgets/helper/SpellChecker.cpp
#include "widgets/helper/SpellChecker.hpp"
#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "common/QLogging.hpp"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

namespace chatterino {

SpellChecker::SpellChecker(QObject *parent)
    : QObject(parent)
    , m_enabled(getSettings()->enableSpellCheck.getValue())
    , m_currentLanguage("en-US")
#ifdef Q_OS_WIN
    , m_spellCheckerFactory(nullptr)
    , m_spellChecker(nullptr)
#elif defined(Q_OS_MACOS)
    , m_nsSpellChecker(nullptr)
#elif defined(Q_OS_LINUX)
    , m_hunspell(nullptr)
#endif
{
    this->initializePlatformSpellChecker();
    
    // Load personal dictionary
    this->loadPersonalDictionary();
    
    // Connect settings changes
    getSettings()->enableSpellCheck.connect([this](bool enabled) {
        this->setEnabled(enabled);
    });
}

SpellChecker::~SpellChecker()
{
    this->cleanup();
}

void SpellChecker::initializePlatformSpellChecker()
{
#ifdef Q_OS_WIN
    // Windows Spell Checking API
    HRESULT hr = CoCreateInstance(CLSID_SpellCheckerFactory, nullptr, 
                                 CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_spellCheckerFactory));
    if (SUCCEEDED(hr))
    {
        LPCWSTR languageTag = L"en-US";
        hr = m_spellCheckerFactory->CreateSpellChecker(languageTag, &m_spellChecker);
        if (!SUCCEEDED(hr))
        {
            qCWarning(chatterinoSpellCheck) << "Failed to create Windows spell checker";
        }
    }
#elif defined(Q_OS_MACOS)
    // macOS NSSpellChecker
    m_nsSpellChecker = [NSSpellChecker sharedSpellChecker];
#elif defined(Q_OS_LINUX)
    // Hunspell for Linux
    QString dictPath = "/usr/share/hunspell/en_US";
    if (QFile::exists(dictPath + ".dic") && QFile::exists(dictPath + ".aff"))
    {
        m_hunspell = new Hunspell((dictPath + ".aff").toUtf8().constData(),
                                 (dictPath + ".dic").toUtf8().constData());
    }
#endif
}

bool SpellChecker::isWordCorrect(const QString &word)
{
    if (!m_enabled || word.isEmpty() || word.length() < 2)
        return true;

    // Check cache first
    QMutexLocker locker(&m_cacheMutex);
    if (m_wordCache.contains(word))
        return m_wordCache[word];

    // Check personal dictionary
    if (m_personalDictionary.contains(word, Qt::CaseInsensitive))
    {
        m_wordCache[word] = true;
        return true;
    }

    bool correct = false;

#ifdef Q_OS_WIN
    if (m_spellChecker)
    {
        LPCWSTR wordPtr = reinterpret_cast<LPCWSTR>(word.utf16());
        IEnumSpellingError *errors = nullptr;
        HRESULT hr = m_spellChecker->Check(wordPtr, &errors);
        if (SUCCEEDED(hr))
        {
            ISpellingError *error = nullptr;
            hr = errors->Next(&error);
            correct = (hr == S_FALSE); // S_FALSE means no more errors
            if (error) error->Release();
            errors->Release();
        }
    }
#elif defined(Q_OS_MACOS)
    if (m_nsSpellChecker)
    {
        NSString *nsWord = word.toNSString();
        NSRange range = [m_nsSpellChecker checkSpellingOfString:nsWord
                                                     startingAt:0];
        correct = (range.location == NSNotFound);
    }
#elif defined(Q_OS_LINUX)
    if (m_hunspell)
    {
        correct = m_hunspell->spell(word.toUtf8().constData());
    }
#endif

    // Cache result
    m_wordCache[word] = correct;
    return correct;
}

QStringList SpellChecker::suggestions(const QString &word)
{
    QStringList suggestions;
    
#ifdef Q_OS_WIN
    if (m_spellChecker)
    {
        LPCWSTR wordPtr = reinterpret_cast<LPCWSTR>(word.utf16());
        IEnumString *suggestionEnum = nullptr;
        HRESULT hr = m_spellChecker->Suggest(wordPtr, &suggestionEnum);
        if (SUCCEEDED(hr))
        {
            LPOLESTR suggestion = nullptr;
            while (suggestionEnum->Next(1, &suggestion, nullptr) == S_OK)
            {
                suggestions << QString::fromWCharArray(suggestion);
                CoTaskMemFree(suggestion);
            }
            suggestionEnum->Release();
        }
    }
#elif defined(Q_OS_MACOS)
    if (m_nsSpellChecker)
    {
        NSString *nsWord = word.toNSString();
        NSArray *nsSuggestions = [m_nsSpellChecker guessesForWordRange:NSMakeRange(0, [nsWord length])
                                                              inString:nsWord
                                                              language:@"en_US"
                                                inSpellDocumentWithTag:0];
        for (NSString *suggestion in nsSuggestions)
        {
            suggestions << QString::fromNSString(suggestion);
        }
    }
#elif defined(Q_OS_LINUX)
    if (m_hunspell)
    {
        std::vector<std::string> hunspellSuggestions = m_hunspell->suggest(word.toUtf8().constData());
        for (const auto &suggestion : hunspellSuggestions)
        {
            suggestions << QString::fromUtf8(suggestion.c_str());
        }
    }
#endif

    return suggestions.mid(0, 5); // Limit to 5 suggestions
}

void SpellChecker::addToPersonalDictionary(const QString &word)
{
    if (!m_personalDictionary.contains(word, Qt::CaseInsensitive))
    {
        m_personalDictionary.append(word);
        this->savePersonalDictionary();
        
        // Update cache
        QMutexLocker locker(&m_cacheMutex);
        m_wordCache[word] = true;
    }
}

void SpellChecker::loadPersonalDictionary()
{
    QString dictPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                      + "/personal_dictionary.txt";
    QFile file(dictPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString word = in.readLine().trimmed();
            if (!word.isEmpty())
            {
                m_personalDictionary.append(word);
            }
        }
    }
}

void SpellChecker::savePersonalDictionary()
{
    QString dictPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dictPath);
    dictPath += "/personal_dictionary.txt";
    
    QFile file(dictPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        for (const QString &word : m_personalDictionary)
        {
            out << word << "\n";
        }
    }
}

} // namespace chatterino
