#include <gtest/gtest.h>
#include "widgets/helper/SpellChecker.hpp"

class SpellCheckerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        spellChecker = new chatterino::SpellChecker();
        spellChecker->setEnabled(true);
    }
    
    void TearDown() override
    {
        delete spellChecker;
    }
    
    chatterino::SpellChecker *spellChecker;
};

TEST_F(SpellCheckerTest, CorrectWordsPassCheck)
{
    EXPECT_TRUE(spellChecker->isWordCorrect("hello"));
    EXPECT_TRUE(spellChecker->isWordCorrect("world"));
    EXPECT_TRUE(spellChecker->isWordCorrect("chat"));
}

TEST_F(SpellCheckerTest, IncorrectWordsFailCheck)
{
    EXPECT_FALSE(spellChecker->isWordCorrect("helo"));
    EXPECT_FALSE(spellChecker->isWordCorrect("wrold"));
    EXPECT_FALSE(spellChecker->isWordCorrect("speling"));
}

TEST_F(SpellCheckerTest, PersonalDictionaryWorks)
{
    QString customWord = "chatterino";
    EXPECT_FALSE(spellChecker->isWordCorrect(customWord));
    
    spellChecker->addToPersonalDictionary(customWord);
    EXPECT_TRUE(spellChecker->isWordCorrect(customWord));
}
