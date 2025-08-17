// src/widgets/helper/SpellCheckHighlighter.cpp
#include "widgets/helper/SpellCheckHighlighter.hpp"
#include "widgets/helper/SpellChecker.hpp"
#include <QTextDocument>
#include <QRegularExpression>

namespace chatterino {

SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument *parent, SpellChecker *spellChecker)
    : QSyntaxHighlighter(parent)
    , m_spellChecker(spellChecker)
    , m_rehighlightTimer(new QTimer(this))
{
    // Configure misspelled word format
    m_misspelledFormat.setUnderlineColor(QColor(Qt::red));
    m_misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    
    // Connect spell checker signals
    connect(m_spellChecker, &SpellChecker::enabledChanged,
            this, &SpellCheckHighlighter::onSpellCheckEnabledChanged);
    
    // Configure rehighlight timer to avoid too frequent updates
    m_rehighlightTimer->setSingleShot(true);
    m_rehighlightTimer->setInterval(100);
    connect(m_rehighlightTimer, &QTimer::timeout, this, &QSyntaxHighlighter::rehighlight);
}

void SpellCheckHighlighter::highlightBlock(const QString &text)
{
    if (!m_spellChecker || !m_spellChecker->isEnabled())
        return;

    // Regular expression to match words (letters, numbers, apostrophes)
    QRegularExpression wordRegex("\\b[A-Za-z']+\\b");
    QRegularExpressionMatchIterator i = wordRegex.globalMatch(text);
    
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(0);
        
        // Skip very short words, numbers, and common chat elements
        if (word.length() < 3 || word.startsWith("@") || word.contains(QRegularExpression("^[0-9]+$")))
            continue;
            
        if (!m_spellChecker->isWordCorrect(word))
        {
            setFormat(match.capturedStart(), match.capturedLength(), m_misspelledFormat);
        }
    }
}

void SpellCheckHighlighter::onSpellCheckEnabledChanged(bool enabled)
{
    Q_UNUSED(enabled)
    m_rehighlightTimer->start();
}

} // namespace chatterino
