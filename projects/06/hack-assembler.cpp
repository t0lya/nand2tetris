#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <map>

using namespace std;

class Parser
{
public:
    enum CommandType
    {
        L_COMMAND,
        A_COMMAND,
        C_COMMAND
    };

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
        {
            return;
        }

        getline(m_file, m_line);
        cleanLine(m_line);

        if (m_line.empty())
        {
            advance();
        }
    }

    void reset()
    {
        m_file.clear();
        m_file.seekg(0);
    }

    CommandType commandType()
    {
        string first = m_line.substr(0, 1);

        if (first == "@")
        {
            return A_COMMAND;
        }
        else if (first == "(")
        {
            return L_COMMAND;
        }
        else
        {
            return C_COMMAND;
        }
    }

    string symbol()
    {
        switch (commandType())
        {
        case A_COMMAND:
            return m_line.substr(1);

        case L_COMMAND:
            return m_line.substr(1, m_line.size() - 2);

        case C_COMMAND:
        default:
            return "";
        }
    }

    string dest()
    {
        if (commandType() == C_COMMAND)
        {
            int equalPos = m_line.find("=");
            return equalPos > 0 ? m_line.substr(0, equalPos) : "";
        }
        else
        {
            return "";
        }
    }

    string comp()
    {
        if (commandType() != C_COMMAND)
        {
            return "";
        }

        int equalPos = m_line.find("=");
        int sColonPos = m_line.find(";");
        return m_line.substr(equalPos < 0 ? 0 : equalPos + 1, sColonPos);
    }

    string jump()
    {
        if (commandType() == C_COMMAND)
        {
            int sColPos = m_line.find(";");
            return sColPos > 0 ? m_line.substr(sColPos + 1) : "";
        }
        else
        {
            return "";
        }
    }

private:
    void cleanLine(string &input)
    {
        regex r("\\s+|//.*");
        input = regex_replace(input, r, "");
    }
};

struct Code
{
    static map<string, string> destMap;
    static map<string, string> compMap;
    static map<string, string> jumpMap;

    static map<string, string> createDestMap()
    {
        map<string, string> tmp;
        tmp[""] = "000";
        tmp["M"] = "001";
        tmp["D"] = "010";
        tmp["MD"] = "011";
        tmp["A"] = "100";
        tmp["AM"] = "101";
        tmp["AD"] = "110";
        tmp["AMD"] = "111";
        return tmp;
    }

    static string dest(string input)
    {
        return destMap[input];
    }

    static map<string, string> createCompMap()
    {
        map<string, string> tmp;
        tmp["0"] = "0101010";
        tmp["1"] = "0111111";
        tmp["-1"] = "0111010";
        tmp["D"] = "0001100";
        tmp["A"] = "0110000";
        tmp["!D"] = "0001101";
        tmp["!A"] = "0110001";
        tmp["-D"] = "0001111";
        tmp["-A"] = "0110011";
        tmp["D+1"] = "0011111";
        tmp["A+1"] = "0110111";
        tmp["D-1"] = "0001110";
        tmp["A-1"] = "0110010";
        tmp["D+A"] = "0000010";
        tmp["D-A"] = "0010011";
        tmp["A-D"] = "0000111";
        tmp["D&A"] = "0000000";
        tmp["D|A"] = "0010101";
        tmp["M"] = "1110000";
        tmp["!M"] = "1110001";
        tmp["-M"] = "1110011";
        tmp["M+1"] = "1110111";
        tmp["M-1"] = "1110010";
        tmp["D+M"] = "1000010";
        tmp["D-M"] = "1010011";
        tmp["M-D"] = "1000111";
        tmp["D&M"] = "1000000";
        tmp["D|M"] = "1010101";
        return tmp;
    }

    static string comp(string input)
    {
        return compMap[input];
    }

    static map<string, string> createJumpMap()
    {
        map<string, string> tmp;
        tmp[""] = "000";
        tmp["JGT"] = "001";
        tmp["JEQ"] = "010";
        tmp["JGE"] = "011";
        tmp["JLT"] = "100";
        tmp["JNE"] = "101";
        tmp["JLE"] = "110";
        tmp["JMP"] = "111";
        return tmp;
    }

    static string jump(string input)
    {
        return jumpMap[input];
    }
};
map<string, string> Code::destMap = Code::createDestMap();
map<string, string> Code::compMap = Code::createCompMap();
map<string, string> Code::jumpMap = Code::createJumpMap();

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cout << "Invalid argument: specify path to .asm file" << endl;
        return -1;
    }

    string asmFilePath = argv[1];
    Parser parser(asmFilePath);

    map<string, int> symbolMap = {
        {"SP", 0},
        {"LCL", 1},
        {"ARG", 2},
        {"THIS", 3},
        {"THAT", 4},
        {"R0", 0},
        {"R1", 1},
        {"R2", 2},
        {"R3", 3},
        {"R4", 4},
        {"R5", 5},
        {"R6", 6},
        {"R7", 7},
        {"R8", 8},
        {"R9", 9},
        {"R10", 10},
        {"R11", 11},
        {"R12", 12},
        {"R13", 13},
        {"R14", 14},
        {"R15", 15},
        {"SCREEN", 16384},
        {"KBD", 24576}};

    parser.advance();
    int lineCount = 0;
    while (parser.hasMoreCommands())
    {
        if (parser.commandType() == Parser::L_COMMAND)
        {
            symbolMap.insert({parser.symbol(), lineCount});
        }
        else
        {
            lineCount++;
        }

        parser.advance();
    }

    ofstream hackFile;
    string hackFilePath = asmFilePath.substr(0, asmFilePath.find_last_of(".")) + ".hack";
    hackFile.open(hackFilePath);

    parser.reset();
    parser.advance();
    int availableAddress = 16;
    while (parser.hasMoreCommands())
    {
        Parser::CommandType cType = parser.commandType();

        if (cType == Parser::C_COMMAND)
        {
            string c = Code::comp(parser.comp());
            string d = Code::dest(parser.dest());
            string j = Code::jump(parser.jump());
            hackFile << "111" + c + d + j << endl;
        }
        else if (cType == Parser::A_COMMAND && parser.symbol().find_first_not_of("0123456789") == string::npos)
        {
            hackFile << "0" + bitset<15>(stoi(parser.symbol())).to_string() << endl;
        }
        else if (cType == Parser::A_COMMAND)
        {
            string s = parser.symbol();
            if (!symbolMap.count(s))
            {
                symbolMap.insert({s, availableAddress});
                availableAddress++;
            }
            hackFile << "0" + bitset<15>(symbolMap[s]).to_string() << endl;
        }

        parser.advance();
    }

    hackFile.close();

    return 0;
}