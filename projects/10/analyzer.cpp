#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <regex>
#include <unordered_map>

using namespace std;

enum TokenType
{
    KEYWORD,
    SYMBOL,
    IDENTIFIER,
    INT_CONST,
    STRING_CONST
};

enum Keyword
{
    CLASS,
    METHOD,
    FUNCTION,
    CONSTRUCTOR,
    INT,
    BOOLEAN,
    CHAR,
    VOID,
    VAR,
    STATIC,
    FIELD,
    LET,
    DO,
    IF,
    ELSE,
    WHILE,
    RETURN,
    TRUE,
    FALSE,
    _NULL,
    THIS,
    UNDEFINED
};

class Tokenizer
{
private:
    ifstream m_file;
    string m_token = "";
    string m_extract = "";
    size_t m_extractIndex = string::npos;
    const string m_symbols = "{}()[].,;+-*/&|<>=~";

public:
    Tokenizer(const string &file)
    {
        m_file.open(file);
    }

    ~Tokenizer()
    {
        m_file.close();
    }

    bool hasMoreTokens()
    {
        return !m_file.eof();
    }

    void advance()
    {
        if (m_extractIndex == string::npos)
        {
            m_file >> m_extract;
            m_extractIndex = 0;
        }

        skipComments();

        if (m_extract[m_extractIndex] == '"')
        {
            string tail;
            getline(m_file, tail, '"');
            m_token = m_extract.substr(m_extractIndex) + tail + "\"";
            m_extractIndex = string::npos;
            return;
        }

        size_t end = m_extract.find_first_of(m_symbols + '"', m_extractIndex);
        if (end == m_extractIndex)
        {
            m_token = m_extract[m_extractIndex];
            m_extractIndex = (m_extractIndex + 1 >= m_extract.length()) ? string::npos : m_extractIndex + 1;
        }
        else
        {
            m_token = m_extract.substr(m_extractIndex, end - m_extractIndex);
            m_extractIndex = end;
        }
    }

    TokenType tokenType()
    {
        if (m_symbols.find(m_token[0]) != string::npos)
        {
            return TokenType::SYMBOL;
        }

        if (regex_match(m_token, regex("^(class|constructor|function|method|field|static|var|int|char|boolean|void|true|false|null|this|let|do|if|else|while|return)$")))
        {
            return TokenType::KEYWORD;
        }

        if (regex_match(m_token, regex("^[0-9]+$")))
        {
            return TokenType::INT_CONST;
        }

        if (m_token[0] == '"' && m_token.back() == '"')
        {
            return TokenType::STRING_CONST;
        }

        return TokenType::IDENTIFIER;
    }

    Keyword keyWord()
    {
        if (tokenType() == TokenType::KEYWORD)
        {
            static const unordered_map<string, Keyword> keywordMap = {
                {"class", Keyword::CLASS},
                {"constructor", Keyword::CONSTRUCTOR},
                {"function", Keyword::FUNCTION},
                {"method", Keyword::METHOD},
                {"field", Keyword::FIELD},
                {"static", Keyword::STATIC},
                {"var", Keyword::VAR},
                {"int", Keyword::INT},
                {"char", Keyword::CHAR},
                {"boolean", Keyword::BOOLEAN},
                {"void", Keyword::VOID},
                {"true", Keyword::TRUE},
                {"false", Keyword::FALSE},
                {"null", Keyword::_NULL},
                {"this", Keyword::THIS},
                {"let", Keyword::LET},
                {"do", Keyword::DO},
                {"if", Keyword::IF},
                {"else", Keyword::ELSE},
                {"while", Keyword::WHILE},
                {"return", Keyword::RETURN},
            };

            return keywordMap.at(m_token);
        };

        return Keyword::UNDEFINED;
    }

    char symbol()
    {
        if (tokenType() == TokenType::SYMBOL)
        {
            return m_token[0];
        }

        return 0;
    }

    string identifier()
    {
        if (tokenType() == TokenType::IDENTIFIER)
        {
            return m_token;
        }

        return "";
    }

    int intVal()
    {
        if (tokenType() == TokenType::INT_CONST)
        {
            return stoi(m_token);
        }

        return 0;
    }

    string stringVal()
    {
        if (tokenType() == TokenType::STRING_CONST)
        {
            return m_token.substr(1, m_token.length() - 2);
        }

        return "";
    }

private:
    void skipComments()
    {
        if (m_extract.rfind("//", 0) == 0)
        {
            m_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            m_file >> m_extract;
            skipComments();
        }
        else if (m_extract.rfind("/*", 0) == 0)
        {
            while (m_extract.find("*/") == string::npos)
            {
                m_file >> m_extract;
            }

            if (m_extract.substr(m_extract.length() - 2) == "*/")
            {
                m_file >> m_extract;
            }
            else
            {
                m_extractIndex = m_extract.find("*/") + 2;
            }

            skipComments();
        }
    }
};

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        cout << "Invalid argument: specify path to .jack file" << endl;
        return -1;
    }

    string jackFilePath = argv[1];
    Tokenizer tokenizer(jackFilePath);

    while (tokenizer.hasMoreTokens())
    {
        tokenizer.advance();
    }

    return 0;
}