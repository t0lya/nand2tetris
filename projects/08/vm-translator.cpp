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
        commands["call"] = C_CALL;
        commands["function"] = C_FUNCTION;
        commands["return"] = C_RETURN;
        commands["label"] = C_LABEL;
        commands["goto"] = C_GOTO;
        commands["if-goto"] = C_IF;

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
        m_file << "// " << (cmd == C_PUSH ? "push" : "pop") << " " << segment << endl;

        if (cmd == C_PUSH && segment == "constant")
        {
            m_file << "@" << index << R"(
D=A
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH && segment == "static")
        {
            m_file << "@" << m_filename << "." << index << R"(
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH && (segment == "pointer" || segment == "temp"))
        {
            m_file << "@R" << to_string(stoi(index) + (segment == "pointer" ? 3 : 5)) << R"(
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
            return;
        }
        else if (cmd == C_PUSH)
        {
            m_file << "@" << index << R"(
D=A
@)" << m_symbols.find(segment)->second
                   << R"(
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
            m_file << R"(@SP
AM=M-1
D=M
@)" << m_filename << "."
                   << index << endl
                   << "M=D" << endl;
            return;
        }
        else if (cmd == C_POP && (segment == "pointer" || segment == "temp"))
        {
            m_file << R"(@SP
AM=M-1
D=M
@R)" << to_string(stoi(index) + (segment == "pointer" ? 3 : 5))
                   << endl
                   << "M=D" << endl;
            return;
        }
        else if (cmd == C_POP)
        {
            m_file << "@" << index << R"(
D=A
@)" << m_symbols.find(segment)->second
                   << R"(
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

    void writeInit()
    {
        m_file << R"(@256
D=A
@SP
M=D
)";
        writeCall("Sys.init", 0);
    }

    void writeLabel(const string &label)
    {
        m_file << "(" << label << ")" << endl;
    }

    void writeGoto(const string &label)
    {
        m_file << "// goto " << label << endl
               << "@" << label << endl
               << "0;JMP" << endl;
    }

    void writeIf(const string &label)
    {
        m_file << "// if-goto " << label << R"(
@SP
AM=M-1
D=M
@)" << label << endl
               << "D;JNE" << endl;
    }

    void writeCall(const string &functionName, int numArgs)
    {
        m_count++;
        const string retAddress = "RETURN_ADDRESS_" + to_string(m_count);
        pushAddress(retAddress);
        pushData("LCL");
        pushData("ARG");
        pushData("THIS");
        pushData("THAT");
        setAddress("ARG", "SP", -5 - numArgs);
        setAddress("LCL", "SP");
        writeGoto(functionName);
        writeLabel(retAddress);
    }

    void writeReturn()
    {
        setAddress("R15", "LCL");
        setData("R14", "R15", -5);
        writePushPop(C_POP, "argument", "0");
        setAddress("SP", "ARG", 1);
        setData("THAT", "R15", -1);
        setData("THIS", "R15", -2);
        setData("ARG", "R15", -3);
        setData("LCL", "R15", -4);
        m_file << R"(@R14
A=M
0;JMP)" << endl;
    }

    void writeFunction(const string &functionName, int numLocals)
    {
        writeLabel(functionName);
        for (size_t i = 0; i < numLocals; i++)
        {
            writePushPop(C_PUSH, "constant", "0");
        }
    }

    void setFileName(const string &filename)
    {
        m_filename = filename;
    }

private:
    void pushAddress(const string &label)
    {
        m_file << "@" << label <<
            R"(
D=A
@SP
AM=M+1
A=A-1
M=D)" << endl;
    }

    void pushData(const string &label)
    {
        m_file << "@" << label << R"(
D=M
@SP
AM=M+1
A=A-1
M=D)" << endl;
    }

    void setAddress(const string &dest, const string &address, int offset = 0)
    {
        bool pos = offset > 0;
        offset = abs(offset);

        m_file << "@" << to_string(offset) << endl
               << "D=A" << endl
               << "@" << address << endl
               << (pos ? "D=D+M" : "D=M-D") << endl
               << "@" << dest << endl
               << "M=D" << endl;
    }

    void setData(const string &dest, const string &address, int offset = 0)
    {
        bool pos = offset > 0;
        offset = abs(offset);

        m_file << "@" << to_string(offset) << endl
               << "D=A" << endl
               << "@" << address << endl
               << (pos ? "A=D+M" : "A=M-D") << endl
               << "D=M" << endl
               << "@" << dest << endl
               << "M=D" << endl;
    }

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

void handleFile(const string &vmFilePath, CodeWriter &codeWriter)
{
    Parser parser(vmFilePath);

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
        else if (cType == C_CALL)
        {
            codeWriter.writeCall(parser.arg1(), stoi(parser.arg2()));
        }
        else if (cType == C_FUNCTION)
        {
            codeWriter.writeFunction(parser.arg1(), stoi(parser.arg2()));
        }
        else if (cType == C_GOTO)
        {
            codeWriter.writeGoto(parser.arg1());
        }
        else if (cType == C_IF)
        {
            codeWriter.writeIf(parser.arg1());
        }
        else if (cType == C_LABEL)
        {
            codeWriter.writeLabel(parser.arg1());
        }
        else if (cType == C_RETURN)
        {
            codeWriter.writeReturn();
        }

        parser.advance();
    }
}

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        cout << "Invalid argument: specify path to .vm file" << endl;
        return -1;
    }

    if (filesystem::is_directory(argv[1]))
    {
        string asmFilePath = (string)argv[1] + "/" + (string)filesystem::path(argv[1]).filename() + ".asm";
        CodeWriter codeWriter(asmFilePath);
        codeWriter.writeInit();

        auto directories = filesystem::directory_iterator(argv[1]);
        for (const auto &entry : directories)
        {
            if (entry.path().extension() == ".vm")
            {
                codeWriter.setFileName(entry.path().filename());
                handleFile(entry.path().string(), codeWriter);
            }
        }
    }
    else
    {
        string vmFilePath = argv[1];
        string asmFilePath = vmFilePath.substr(0, vmFilePath.find_last_of(".")) + ".asm";

        CodeWriter codeWriter(asmFilePath);
        codeWriter.writeInit();

        handleFile(vmFilePath, codeWriter);
    }

    return 0;
}