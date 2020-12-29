#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include <regex>

using namespace std;

enum CommandType
{
    C_ARITHMETIC,
    C_PUSH,
    C_POP,
    C_LABEL,
    C_GOTO,
    C_IF,
    C_FUNCTION,
    C_RETURN,
    C_CALL
};

class Parser
{
private:
    ifstream m_file;
    string m_line;

public:
    Parser(string filename)
    {
        m_file.open(filename);
    }

    ~Parser()
    {
        m_file.close();
    }

    bool hasMoreCommands()
    {
        return !m_file.eof();
    }

    void advance()
    {
        if (!hasMoreCommands())
            return;

        do
        {
            getline(m_file, m_line);
            cleanLine(m_line);
        } while (hasMoreCommands() && m_line.empty());
    }

    CommandType commandType()
    {
        map<string, CommandType> commands;
        commands["add"] = C_ARITHMETIC;
        commands["sub"] = C_ARITHMETIC;
        commands["neg"] = C_ARITHMETIC;
        commands["eq"] = C_ARITHMETIC;
        commands["gt"] = C_ARITHMETIC;
        commands["lt"] = C_ARITHMETIC;
        commands["and"] = C_ARITHMETIC;
        commands["or"] = C_ARITHMETIC;
        commands["not"] = C_ARITHMETIC;
        commands["push"] = C_PUSH;
        commands["pop"] = C_POP;

        return commands.find(m_line.substr(0, m_line.find(" ")))->second;
    }

    string arg1()
    {
        if (commandType() == C_RETURN)
        {
            return "";
        }

        auto first_pos = m_line.find(" ");

        if (commandType() == C_ARITHMETIC)
        {
            return m_line.substr(0, first_pos);
        }

        return m_line.substr(first_pos + 1, m_line.find(" ", first_pos + 1) - first_pos - 1);
    }

    string arg2()
    {
        CommandType cmd = commandType();
        if (cmd == C_PUSH || cmd == C_POP || cmd == C_FUNCTION || cmd == C_CALL)
        {
            auto first_pos = m_line.find(" ");
            auto second_pos = m_line.find(" ", first_pos + 1);
            auto third_pos = m_line.find(" ", second_pos + 1);
            return m_line.substr(second_pos + 1, third_pos - second_pos - 1);
        }
        else
        {
            return "";
        }
    }

private:
    void cleanLine(string &input)
    {
        regex r("//.*|\r|\n");
        input = regex_replace(input, r, "");
    }
};

class CodeWriter
{
private:
    ofstream m_file;
    string m_filename;
    int m_count = 0;
    map<string, string> m_symbols;

public:
    CodeWriter(string filename)
    {
        m_file.open(filename);

        filesystem::path tmp(filename);
        m_filename = tmp.stem();

        m_symbols["local"] = "LCL";
        m_symbols["argument"] = "ARG";
        m_symbols["this"] = "THIS";
        m_symbols["that"] = "THAT";
        m_symbols["temp"] = "R5";
    }

    ~CodeWriter()
    {
        m_file.close();
    }

    void writeArithmetic(const string &input)
    {

        if (input == "add")
        {
            m_file << "// add\n" + getOperatorSnippet("D+M") << endl;
            return;
        }

        if (input == "sub")
        {
            m_file << "// sub\n" + getOperatorSnippet("M-D") << endl;
            return;
        }

        if (input == "neg")
        {
            m_file << R"(// neg
@SP
A=M-1
M=-M)" << endl;
            return;
        }

        if (input == "and")
        {
            m_file
                << "// and\n" + getOperatorSnippet("D&M") << endl;
            return;
        }

        if (input == "or")
        {
            m_file << "// or\n" + getOperatorSnippet("D|M") << endl;
            return;
        }

        if (input == "not")
        {
            m_file << R"(// not
@SP
A=M-1
M=!M)" << endl;
            return;
        }

        m_count++;
        string label = "AR_";
        label += to_string(m_count);

        if (input == "eq")
        {
            m_file << "// eq\n" + getComparisonSnippet(label, "JNE") << endl;
            return;
        }

        if (input == "gt")
        {
            m_file << "// gt\n" + getComparisonSnippet(label, "JLE") << endl;
            return;
        }

        if (input == "lt")
        {
            m_file << "// lt\n" + getComparisonSnippet(label, "JGE") << endl;
            return;
        }
    }

    void writePushPop(const CommandType &cmd, const string &segment, const string &index)
    {
        if (cmd == C_PUSH && segment == "constant")
        {
            m_file << "// push constant\n@" + index +
                          R"(
D=A
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH && segment == "static")
        {
            m_file << "// push static\n@" + m_filename + "." +
                          index +
                          R"(
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH && segment == "pointer")
        {
            m_file << R"(// push pointer
@R)" + to_string(stoi(index) + 3) +
                          R"(
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH)
        {
            m_file << "// push\n@" + index +
                          R"(
D=A
@)" + m_symbols.find(segment)->second +
                          R"(
A=D+M
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_POP && segment == "static")
        {
            m_file << R"(// pop static
@SP
AM=M-1
D=M
@)" + m_filename +
                          "." +
                          index +
                          "\nM=D"
                   << endl;
            return;
        }
        else if (cmd == C_POP && segment == "pointer")
        {
            m_file << R"(// pop pointer
@SP
AM=M-1
D=M
@R)" + to_string(stoi(index) + 3) +
                          R"(
M=D)" << endl;
            return;
        }
        else if (cmd == C_POP)
        {
            m_file << "// pop\n@" + index +
                          R"(
D=A
@)" + m_symbols.find(segment)->second +
                          R"(
D=D+M
@R13
M=D
@SP
AM=M-1
D=M
@R13
A=M
M=D)" << endl;
            return;
        }
    }

private:
    string getComparisonSnippet(const string &label, const string &jump)
    {
        return R"(@SP
AM=M-1
D=M
A=A-1
D=M-D
M=0
@)" + label +
               R"(
D;)" + jump +
               R"#(
@SP
A=M-1
M=-1
()#" + label +
               ")";
    }

    string getOperatorSnippet(const string &line)
    {
        return R"(@SP
AM=M-1
D=M
A=A-1
M=)" + line;
    }
};

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        cout << "Invalid argument: specify path to .vm file" << endl;
        return -1;
    }

    string vmFilePath = argv[1];
    Parser parser(vmFilePath);
    string asmFilePath = vmFilePath.substr(0, vmFilePath.find_last_of(".")) + ".asm";
    CodeWriter codeWriter(asmFilePath);

    while (parser.hasMoreCommands())
    {
        CommandType cType = parser.commandType();

        if (cType == C_ARITHMETIC)
        {
            codeWriter.writeArithmetic(parser.arg1());
        }
        else if (cType == C_PUSH || cType == C_POP)
        {
            codeWriter.writePushPop(cType, parser.arg1(), parser.arg2());
        }

        parser.advance();
    }

    return 0;
}