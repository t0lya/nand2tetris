#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <regex>
#include <unordered_map>
#include <filesystem>

using namespace std;

enum class TokenType
{
    KEYWORD,
    SYMBOL,
    IDENTIFIER,
    INT_CONST,
    STRING_CONST
};

enum class Keyword
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

enum class IdentifierKind
{
    STATIC,
    FIELD,
    ARG,
    VAR,
    NONE
};

struct Identifier
{
    string type;
    IdentifierKind kind;
    int index;
};

class SymbolTable
{
private:
    unordered_map<string, Identifier> classIdentifiers;
    unordered_map<string, Identifier> subroutineIdentifiers;
    int staticCount = 0;
    int fieldCount = 0;
    int argCount = 0;
    int localCount = 0;

public:
    void startSubroutine()
    {
        argCount = 0;
        localCount = 0;
        subroutineIdentifiers.clear();
    }

    void define(const string &name, const string &type, IdentifierKind kind)
    {
        if (kind == IdentifierKind::STATIC || kind == IdentifierKind::FIELD)
        {
            int index = kind == IdentifierKind::STATIC ? staticCount++ : fieldCount++;
            classIdentifiers.insert(pair<string, Identifier>{name, Identifier{type, kind, index}});
        }
        else if (kind == IdentifierKind::ARG || kind == IdentifierKind::VAR)
        {
            int index = kind == IdentifierKind::ARG ? argCount++ : localCount++;
            subroutineIdentifiers.insert(pair<string, Identifier>{name, Identifier{type, kind, index}});
        }
    }

    int varCount(IdentifierKind kind)
    {
        switch (kind)
        {
        case IdentifierKind::STATIC:
            return staticCount;

        case IdentifierKind::FIELD:
            return fieldCount;

        case IdentifierKind::ARG:
            return argCount;

        case IdentifierKind::VAR:
            return localCount;

        default:
            return 0;
        }
    }

    IdentifierKind kindOf(const string &name)
    {
        auto srId = subroutineIdentifiers.find(name);
        if (srId != subroutineIdentifiers.end())
        {
            return srId->second.kind;
        }

        auto classId = classIdentifiers.find(name);
        if (classId != classIdentifiers.end())
        {
            return classId->second.kind;
        }

        return IdentifierKind::NONE;
    }

    string typeOf(const string &name)
    {
        auto srId = subroutineIdentifiers.find(name);
        if (srId != subroutineIdentifiers.end())
        {
            return srId->second.type;
        }

        auto classId = classIdentifiers.find(name);
        if (classId != classIdentifiers.end())
        {
            return classId->second.type;
        }

        return "";
    }

    int indexOf(const string &name)
    {
        auto srId = subroutineIdentifiers.find(name);
        if (srId != subroutineIdentifiers.end())
        {
            return srId->second.index;
        }

        auto classId = classIdentifiers.find(name);
        if (classId != classIdentifiers.end())
        {
            return classId->second.index;
        }

        return -1;
    }
};

enum class Segment
{
    CONST,
    ARG,
    LOCAL,
    STATIC,
    THIS,
    THAT,
    POINTER,
    TEMP
};

class VMWriter
{
private:
    ofstream m_file;

public:
    VMWriter(const string &filename)
    {
        m_file.open(filename);
    }

    ~VMWriter()
    {
        m_file.close();
    }

    void writePush(Segment segment, int index)
    {
        m_file << "push " << segmentToString(segment) << " " << to_string(index) << endl;
    }

    void writePop(Segment segment, int index)
    {
        m_file << "pop " << segmentToString(segment) << " " << to_string(index) << endl;
    }

    void writeArithmetic(string command)
    {
        m_file << command << endl;
    }

    void writeLabel(string label)
    {
        m_file << "label " << label << endl;
    }

    void writeGoto(string label)
    {
        m_file << "goto " << label << endl;
    }

    void writeIf(string label)
    {
        m_file << "if-goto " << label << endl;
    }

    void writeCall(string name, int nArgs)
    {
        m_file << "call " << name << " " << to_string(nArgs) << endl;
    }

    void writeFunction(string &name, int nLocals)
    {
        m_file << "function " << name << " " << to_string(nLocals) << endl;
    }

    void writeReturn()
    {
        m_file << "return" << endl;
    }

private:
    string segmentToString(Segment segment)
    {
        switch (segment)
        {
        case Segment::CONST:
            return "constant";

        case Segment::ARG:
            return "argument";

        case Segment::LOCAL:
            return "local";

        case Segment::STATIC:
            return "static";

        case Segment::THIS:
            return "this";

        case Segment::THAT:
            return "that";

        case Segment::POINTER:
            return "pointer";

        case Segment::TEMP:
            return "temp";

        default:
            return "";
        }
    }
};

class CompilationEngine
{
private:
    Tokenizer *tokenizer;
    VMWriter *vmWriter;
    SymbolTable symbolTable;
    string className;
    int controlCount = 0;

public:
    CompilationEngine(Tokenizer *tokenizer, VMWriter *vmWriter) : tokenizer(tokenizer), vmWriter(vmWriter), symbolTable() {}

    void compileClass()
    {
        if (tokenizer->keyWord() == Keyword::CLASS)
        {
            tokenizer->advance();
            className = tokenizer->identifier();
            tokenizer->advance();
            tokenizer->advance();

            while (tokenizer->keyWord() == Keyword::STATIC || tokenizer->keyWord() == Keyword::FIELD)
            {
                compileClassVarDec();
            }

            while (tokenizer->keyWord() == Keyword::CONSTRUCTOR || tokenizer->keyWord() == Keyword::FUNCTION || tokenizer->keyWord() == Keyword::METHOD)
            {
                compileSubroutineDec();
            }

            tokenizer->advance();
        }
    }

    void compileClassVarDec()
    {
        auto kind = tokenizer->keyWord() == Keyword::STATIC ? IdentifierKind::STATIC : IdentifierKind::FIELD;
        tokenizer->advance();
        auto type = tokenizer->token();
        tokenizer->advance();
        auto name = tokenizer->identifier();
        symbolTable.define(name, type, kind);
        tokenizer->advance();

        while (tokenizer->symbol() == ',')
        {
            tokenizer->advance();
            symbolTable.define(tokenizer->identifier(), type, kind);
            tokenizer->advance();
        }

        tokenizer->advance();
    }

    void compileSubroutineDec()
    {
        symbolTable.startSubroutine();
        auto subroutineType = tokenizer->keyWord();
        tokenizer->advance();
        auto returnType = tokenizer->token();
        tokenizer->advance();
        auto functionName = tokenizer->identifier();
        tokenizer->advance();
        tokenizer->advance();

        if (subroutineType == Keyword::METHOD)
        {
            symbolTable.define("this", className, IdentifierKind::ARG);
        }

        compileParameterList();
        tokenizer->advance();

        tokenizer->advance();
        while (tokenizer->keyWord() == Keyword::VAR)
        {
            compileVarDec();
        }
        auto nLocals = symbolTable.varCount(IdentifierKind::VAR);
        auto fullFuncName = className + "." + functionName;
        vmWriter->writeFunction(fullFuncName, nLocals);

        if (subroutineType == Keyword::METHOD)
        {
            vmWriter->writePush(Segment::ARG, 0);
            vmWriter->writePop(Segment::POINTER, 0);
        }
        else if (subroutineType == Keyword::CONSTRUCTOR)
        {
            vmWriter->writePush(Segment::CONST, symbolTable.varCount(IdentifierKind::FIELD));
            string alloc = "Memory.alloc";
            vmWriter->writeCall(alloc, 1);
            vmWriter->writePop(Segment::POINTER, 0);
        }

        compileStatements();
        tokenizer->advance();
    }

    void compileParameterList()
    {
        if (isType())
        {
            auto argType = tokenizer->token();
            tokenizer->advance();
            auto argName = tokenizer->identifier();
            symbolTable.define(argName, argType, IdentifierKind::ARG);
            tokenizer->advance();

            while (tokenizer->symbol() == ',')
            {
                tokenizer->advance();
                auto argType = tokenizer->token();
                tokenizer->advance();
                auto argName = tokenizer->identifier();
                symbolTable.define(argName, argType, IdentifierKind::ARG);
                tokenizer->advance();
            }
        }
    }

    void compileVarDec()
    {
        if (tokenizer->keyWord() == Keyword::VAR)
        {
            tokenizer->advance();
            auto varType = tokenizer->token();
            tokenizer->advance();
            auto varName = tokenizer->identifier();
            symbolTable.define(varName, varType, IdentifierKind::VAR);
            tokenizer->advance();

            while (tokenizer->symbol() == ',')
            {
                tokenizer->advance();
                symbolTable.define(tokenizer->identifier(), varType, IdentifierKind::VAR);
                tokenizer->advance();
            }

            tokenizer->advance();
        }
    }

    void compileStatements()
    {
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
    }

    void compileDo()
    {
        if (tokenizer->keyWord() == Keyword::DO)
        {
            tokenizer->advance();
            auto subroutineName = tokenizer->identifier();
            tokenizer->advance();
            compileSubroutineCall(subroutineName, true);
            tokenizer->advance();
        }
    }

    void compileLet()
    {
        if (tokenizer->keyWord() != Keyword::LET)
        {
            return;
        }

        tokenizer->advance();
        auto varName = tokenizer->identifier();
        tokenizer->advance();

        if (tokenizer->symbol() == '[')
        {
            tokenizer->advance();
            vmWriter->writePush(toSegment(symbolTable.kindOf(varName)), symbolTable.indexOf(varName));
            compileExpression();
            vmWriter->writeArithmetic("add");
            tokenizer->advance();
            tokenizer->advance();
            compileExpression();
            vmWriter->writePop(Segment::TEMP, 0);
            vmWriter->writePop(Segment::POINTER, 1);
            vmWriter->writePush(Segment::TEMP, 0);
            vmWriter->writePop(Segment::THAT, 0);
        }
        else
        {
            tokenizer->advance();
            compileExpression();
            vmWriter->writePop(toSegment(symbolTable.kindOf(varName)), symbolTable.indexOf(varName));
        }

        tokenizer->advance();
    }

    void compileWhile()
    {
        if (tokenizer->keyWord() != Keyword::WHILE)
        {
            return;
        }

        controlCount++;
        auto startLabel = "WHILE_START_" + to_string(controlCount);
        auto endLabel = "WHILE_END_" + to_string(controlCount);
        vmWriter->writeLabel(startLabel);
        tokenizer->advance();
        tokenizer->advance();
        compileExpression();
        tokenizer->advance();

        vmWriter->writeArithmetic("not");
        vmWriter->writeIf(endLabel);

        tokenizer->advance();
        compileStatements();
        vmWriter->writeGoto(startLabel);
        vmWriter->writeLabel(endLabel);
        tokenizer->advance();
    }

    void compileReturn()
    {
        if (tokenizer->keyWord() != Keyword::RETURN)
        {
            return;
        }

        tokenizer->advance();

        if (tokenizer->symbol() != ';')
        {
            compileExpression();
        }
        else
        {
            vmWriter->writePush(Segment::CONST, 0);
        }

        vmWriter->writeReturn();
        tokenizer->advance();
    }

    void compileIf()
    {
        if (tokenizer->keyWord() != Keyword::IF)
        {
            return;
        }

        controlCount++;
        auto endLabel = "IF_END_" + to_string(controlCount);
        auto falseLabel = "IF_FALSE_" + to_string(controlCount);
        tokenizer->advance();
        tokenizer->advance();
        compileExpression();
        tokenizer->advance();
        vmWriter->writeArithmetic("not");
        vmWriter->writeIf(falseLabel);

        tokenizer->advance();
        compileStatements();
        vmWriter->writeGoto(endLabel);
        vmWriter->writeLabel(falseLabel);
        tokenizer->advance();

        if (tokenizer->keyWord() == Keyword::ELSE)
        {
            tokenizer->advance();
            tokenizer->advance();
            compileStatements();
            tokenizer->advance();
        }

        vmWriter->writeLabel(endLabel);
    }

    void compileExpression()
    {
        compileTerm();

        while (isOp())
        {
            auto symbol = tokenizer->symbol();
            tokenizer->advance();
            compileTerm();

            if (symbol == '*')
            {
                vmWriter->writeCall("Math.multiply", 2);
            }
            else if (symbol == '/')
            {
                vmWriter->writeCall("Math.divide", 2);
            }
            else
            {
                vmWriter->writeArithmetic(toArithmetic(symbol));
            }
        }
    }

    void compileTerm()
    {
        switch (tokenizer->tokenType())
        {
        case TokenType::IDENTIFIER:
        {
            auto identifier = tokenizer->identifier();
            tokenizer->advance();
            char symbol = tokenizer->symbol();
            if (symbol == '[')
            {
                tokenizer->advance();
                vmWriter->writePush(toSegment(symbolTable.kindOf(identifier)), symbolTable.indexOf(identifier));
                compileExpression();
                vmWriter->writeArithmetic("add");
                vmWriter->writePop(Segment::POINTER, 1);
                vmWriter->writePush(Segment::THAT, 0);
                tokenizer->advance();
            }
            else if (symbol == '(' || symbol == '.')
            {
                compileSubroutineCall(identifier, false);
            }
            else
            {
                vmWriter->writePush(toSegment(symbolTable.kindOf(identifier)), symbolTable.indexOf(identifier));
            }

            break;
        }

        case TokenType::STRING_CONST:
        {
            auto str = tokenizer->stringVal();
            auto strLen = str.length();

            vmWriter->writePush(Segment::CONST, strLen);
            vmWriter->writeCall("String.new", 1);

            for (auto &chr : str)
            {
                vmWriter->writePush(Segment::CONST, (int)chr);
                vmWriter->writeCall("String.appendChar", 2);
            }

            tokenizer->advance();
            break;
        }

        case TokenType::SYMBOL:
        {
            char symbol = tokenizer->symbol();
            tokenizer->advance();

            if (symbol == '-')
            {

                compileTerm();
                vmWriter->writeArithmetic("neg");
            }
            else if (symbol == '~')
            {
                compileTerm();
                vmWriter->writeArithmetic("not");
            }
            else
            {
                compileExpression();
                tokenizer->advance();
            }

            break;
        }

        case TokenType::INT_CONST:
        {
            vmWriter->writePush(Segment::CONST, tokenizer->intVal());
            tokenizer->advance();
            break;
        }

        case TokenType::KEYWORD:
        {
            auto keyword = tokenizer->keyWord();

            if (keyword == Keyword::TRUE)
            {
                vmWriter->writePush(Segment::CONST, 1);
                vmWriter->writeArithmetic("neg");
            }
            else if (keyword == Keyword::FALSE || keyword == Keyword::_NULL)
            {
                vmWriter->writePush(Segment::CONST, 0);
            }
            else if (keyword == Keyword::THIS)
            {
                vmWriter->writePush(Segment::POINTER, 0);
            }

            tokenizer->advance();
            break;
        }
        }
    }

    int compileExpressionList()
    {
        int nArgs = 0;

        if (tokenizer->symbol() != ')')
        {
            compileExpression();
            nArgs++;
        }

        while (tokenizer->symbol() == ',')
        {
            tokenizer->advance();
            compileExpression();
            nArgs++;
        }

        return nArgs;
    }

private:
    void compileSubroutineCall(string subroutineName, bool isVoid)
    {
        auto funcName = className + "." + subroutineName;
        auto hasThisArg = symbolTable.kindOf(subroutineName) != IdentifierKind::NONE;
        auto symbol = tokenizer->symbol();

        if (symbol == '.')
        {
            tokenizer->advance();
            auto cName = hasThisArg ? symbolTable.typeOf(subroutineName) : subroutineName;
            funcName = cName + "." + tokenizer->identifier();
            tokenizer->advance();
        }
        else
        {
            hasThisArg = true;
        }

        tokenizer->advance();
        if (hasThisArg && symbol != '.')
        {
            vmWriter->writePush(Segment::POINTER, 0);
        }
        else if (hasThisArg && symbol == '.')
        {
            vmWriter->writePush(toSegment(symbolTable.kindOf(subroutineName)), symbolTable.indexOf(subroutineName));
        }
        int nArgs = compileExpressionList();
        if (hasThisArg)
            nArgs++;

        vmWriter->writeCall(funcName, nArgs);

        if (isVoid)
        {
            vmWriter->writePop(Segment::TEMP, 0);
        }

        tokenizer->advance();
    }

    Segment toSegment(IdentifierKind kind)
    {
        switch (kind)
        {
        case IdentifierKind::STATIC:
            return Segment::STATIC;

        case IdentifierKind::FIELD:
            return Segment::THIS;

        case IdentifierKind::ARG:
            return Segment::ARG;

        case IdentifierKind::VAR:
            return Segment::LOCAL;

        default:
            return Segment::TEMP;
        }
    }

    string toArithmetic(char op)
    {
        switch (op)
        {
        case '+':
            return "add";

        case '-':
            return "sub";

        case '*':
            return "add";

        case '/':
            return "add";

        case '&':
            return "and";

        case '|':
            return "or";

        case '<':
            return "lt";

        case '>':
            return "gt";

        case '=':
            return "eq";

        default:
            return "";
        }
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
};

void compile(const string &jackFilePath, const string &vmFilePath)
{
    Tokenizer tokenizer(jackFilePath);
    VMWriter vmWriter(vmFilePath);
    CompilationEngine compiler(&tokenizer, &vmWriter);
    tokenizer.advance();
    compiler.compileClass();
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
                string vmFilePath = jackFilePath.substr(0, jackFilePath.find_last_of(".")) + ".vm";
                compile(jackFilePath, vmFilePath);
            }
        }
    }
    else
    {
        string jackFilePath = argv[1];
        string vmFilePath = jackFilePath.substr(0, jackFilePath.find_last_of(".")) + ".vm";
        compile(jackFilePath, vmFilePath);
    }

    return 0;
}