#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <regex>
#include <unordered_map>
#include <filesystem>
#include "./pugixml.hpp"

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
            m_file >> ws;
            m_extractIndex = 0;
        }

        skipComments();

        if (m_extract[m_extractIndex] == '"')
        {
            string tail;
            getline(m_file, tail, '"');
            // TODO: Investigate skipped whitespace between extracts
            m_token = m_extract.substr(m_extractIndex) + " " + tail + "\"";
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

    string token()
    {
        return m_token;
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

class CompilationEngine
{
private:
    Tokenizer *tokenizer;
    pugi::xml_node node;

public:
    CompilationEngine(Tokenizer *tokenizer, pugi::xml_document &doc)
    {
        this->tokenizer = tokenizer;
        this->node = doc;
    }

    void compileClass()
    {
        if (tokenizer->keyWord() == Keyword::CLASS)
        {
            auto tmpNode = node;
            node = node.append_child("class");

            compileKeyword();
            compileIdentifier();
            compileSymbol();

            while (tokenizer->keyWord() == Keyword::STATIC || tokenizer->keyWord() == Keyword::FIELD)
            {
                compileClassVarDec();
            }

            while (tokenizer->keyWord() == Keyword::CONSTRUCTOR || tokenizer->keyWord() == Keyword::FUNCTION || tokenizer->keyWord() == Keyword::METHOD)
            {
                compileSubroutineDec();
            }

            compileSymbol();
            node = tmpNode;
        }
    }

    void compileClassVarDec()
    {
        auto tmpNode = node;
        node = node.append_child("classVarDec");
        compileKeyword();
        compileType();
        compileIdentifier();

        while (tokenizer->symbol() == ',')
        {
            compileSymbol();
            compileIdentifier();
        }

        compileSymbol();
        node = tmpNode;
    }

    void compileSubroutineDec()
    {
        auto tmpNode = node;
        node = node.append_child("subroutineDec");
        compileKeyword();
        compileType();
        compileIdentifier();
        compileSymbol();
        compileParameterList();
        compileSymbol();

        node = node.append_child("subroutineBody");
        compileSymbol();
        while (tokenizer->keyWord() == Keyword::VAR)
        {
            compileVarDec();
        }
        compileStatements();
        compileSymbol();

        node = tmpNode;
    }

    void compileParameterList()
    {
        auto tmpNode = node;
        node = node.append_child("parameterList");

        if (isType())
        {
            compileType();
            compileIdentifier();

            while (tokenizer->symbol() == ',')
            {
                compileSymbol();
                compileType();
                compileIdentifier();
            }
        }

        node = tmpNode;
    }

    void compileVarDec()
    {
        auto tmpNode = node;
        node = node.append_child("varDec");

        compileKeyword();
        compileType();
        compileIdentifier();

        while (tokenizer->symbol() == ',')
        {
            compileSymbol();
            compileIdentifier();
        }

        compileSymbol();
        node = tmpNode;
    }

    void compileStatements()
    {
        auto tmpNode = node;
        node = node.append_child("statements");

        while (tokenizer->tokenType() == TokenType::KEYWORD)
        {
            switch (tokenizer->keyWord())
            {
            case Keyword::IF:
                compileIf();
                break;

            case Keyword::DO:
                compileDo();
                break;

            case Keyword::WHILE:
                compileWhile();
                break;

            case Keyword::RETURN:
                compileReturn();
                break;

            case Keyword::LET:
                compileLet();
                break;

            default:
                break;
            }
        }

        node = tmpNode;
    }

    void compileDo()
    {
        auto tmpNode = node;
        node = node.append_child("doStatement");

        compileKeyword();
        compileSubroutineCall();
        compileSymbol();
        node = tmpNode;
    }

    void compileLet()
    {
        auto tmpNode = node;
        node = node.append_child("letStatement");

        compileKeyword();
        compileIdentifier();

        if (tokenizer->symbol() == '[')
        {
            compileSymbol();
            compileExpression();
            compileSymbol();
        }

        compileSymbol();
        compileExpression();
        compileSymbol();
        node = tmpNode;
    }

    void compileWhile()
    {
        auto tmpNode = node;
        node = node.append_child("whileStatement");

        compileKeyword();

        compileSymbol();
        compileExpression();
        compileSymbol();

        compileSymbol();
        compileStatements();
        compileSymbol();

        node = tmpNode;
    }

    void compileReturn()
    {
        auto tmpNode = node;
        node = node.append_child("returnStatement");

        compileKeyword();

        if (tokenizer->symbol() != ';')
        {
            compileExpression();
        }

        compileSymbol();

        node = tmpNode;
    }

    void compileIf()
    {
        auto tmpNode = node;
        node = node.append_child("ifStatement");

        compileKeyword();
        compileSymbol();
        compileExpression();
        compileSymbol();

        compileSymbol();
        compileStatements();
        compileSymbol();

        if (tokenizer->keyWord() == Keyword::ELSE)
        {
            compileKeyword();
            compileSymbol();
            compileStatements();
            compileSymbol();
        }

        node = tmpNode;
    }

    void compileExpression()
    {
        auto tmpNode = node;
        node = node.append_child("expression");

        compileTerm();

        while (isOp())
        {
            compileSymbol();
            compileTerm();
        }

        node = tmpNode;
    }

    void compileTerm()
    {
        auto tmpNode = node;
        node = node.append_child("term");

        switch (tokenizer->tokenType())
        {
        case TokenType::IDENTIFIER:
        {
            compileIdentifier();
            char symbol = tokenizer->symbol();
            if (symbol == '[')
            {
                compileSymbol();
                compileExpression();
                compileSymbol();
            }
            else if (symbol == '(' || symbol == '.')
            {
                compileSubroutineCall(false);
            }
            break;
        }

        case TokenType::STRING_CONST:
        {
            auto token = node.append_child("stringConstant");
            token.text().set(tokenizer->stringVal().c_str());
            tokenizer->advance();
            break;
        }

        case TokenType::SYMBOL:
        {
            char symbol = tokenizer->symbol();
            if (symbol == '-' || symbol == '~')
            {
                compileSymbol();
                compileTerm();
            }
            else
            {
                compileSymbol();
                compileExpression();
                compileSymbol();
            }
            break;
        }

        case TokenType::INT_CONST:
        {
            auto token = node.append_child("integerConstant");
            token.text().set(tokenizer->token().c_str());
            tokenizer->advance();
            break;
        }

        case TokenType::KEYWORD:
        {
            compileKeyword();
            break;
        }
        }

        node = tmpNode;
    }

    void compileExpressionList()
    {
        auto tmpNode = node;
        node = node.append_child("expressionList");

        if (tokenizer->symbol() != ')')
        {
            compileExpression();
        }

        while (tokenizer->symbol() == ',')
        {
            compileSymbol();
            compileExpression();
        }

        node = tmpNode;
    }

private:
    void compileSubroutineCall(bool withIdentifier = true)
    {
        if (withIdentifier)
        {
            compileIdentifier();
        }

        if (tokenizer->symbol() == '.')
        {
            compileSymbol();
            compileIdentifier();
        }

        compileSymbol();
        compileExpressionList();
        compileSymbol();
    }

    bool isOp()
    {
        auto symbol = tokenizer->symbol();
        return symbol == '+' || symbol == '-' || symbol == '*' || symbol == '/' || symbol == '&' || symbol == '|' || symbol == '<' || symbol == '>' || symbol == '=';
    }

    bool isType()
    {
        auto type = tokenizer->keyWord();
        return type == Keyword::INT || type == Keyword::BOOLEAN || type == Keyword::CHAR || tokenizer->tokenType() == TokenType::IDENTIFIER;
    }

    void compileType()
    {
        if (tokenizer->tokenType() == TokenType::IDENTIFIER)
        {
            compileIdentifier();
        }
        else
        {
            compileKeyword();
        }
    }

    void compileKeyword()
    {
        auto keywordNode = node.append_child("keyword");
        keywordNode.text().set(tokenizer->token().c_str());
        tokenizer->advance();
    }

    void compileSymbol()
    {
        auto symbolNode = node.append_child("symbol");
        symbolNode.text().set(tokenizer->token().c_str());
        tokenizer->advance();
    }

    void compileIdentifier()
    {
        auto identifierNode = node.append_child("identifier");
        identifierNode.text().set(tokenizer->identifier().c_str());
        tokenizer->advance();
    }
};

void createXml(const string &jackFilePath, const string &syntaxTreePath)
{
    Tokenizer tokenizer(jackFilePath);
    pugi::xml_document doc;
    CompilationEngine compiler(&tokenizer, doc);
    tokenizer.advance();
    compiler.compileClass();
    doc.save_file(syntaxTreePath.c_str(), "\t", pugi::format_indent | pugi::format_no_declaration | pugi::format_no_empty_element_tags);
}

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        cout << "Invalid argument: specify path to .jack file" << endl;
        return -1;
    }

    if (filesystem::is_directory(argv[1]))
    {
        auto directories = filesystem::directory_iterator(argv[1]);
        for (const auto &entry : directories)
        {
            if (entry.path().extension() == ".jack")
            {
                string jackFilePath = entry.path();
                string syntaxTreePath = jackFilePath.substr(0, jackFilePath.find_last_of(".")) + "SyntaxTree.xml";
                createXml(jackFilePath, syntaxTreePath);
            }
        }
    }
    else
    {
        string jackFilePath = argv[1];
        string syntaxTreePath = jackFilePath.substr(0, jackFilePath.find_last_of(".")) + "SyntaxTree.xml";
        createXml(jackFilePath, syntaxTreePath);
    }

    return 0;
}