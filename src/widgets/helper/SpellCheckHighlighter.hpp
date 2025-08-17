// src/widgets/helper/SpellCheckHighlighter.hpp
#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTimer>

namespace chatterino {

class SpellChecker;

class SpellCheckHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SpellCheckHighlighter(QTextDocument *parent, SpellChecker *spellChecker);

protected:
    void highlightBlock(const QString &text) override;

private slots:
    void onSpellCheckEnabledChanged(bool enabled);

private:
    SpellChecker *m_spellChecker;
    QTextCharFormat m_misspelledFormat;
    QTimer *m_rehighlightTimer;
};

} // namespace chatterino
