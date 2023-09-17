#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include "LefDefParser.h"

using namespace LefDefDB;

typedef std::filesystem::directory_iterator dirItr;

class CmdInterpreter
{
  public:
    
    CmdInterpreter(std::shared_ptr<LefDefParser> parser)    // Constructor
      : parser_ (parser) 
    {}

    void readCmd(const std::filesystem::path& cmdfile);     // Read Command (.cmd) file

  private:

    std::shared_ptr<LefDefParser> parser_;                  // SharedPointer of Parser

    // For parsing .cmd file
    std::ifstream             file_;                        // Input File Stream
    std::stringstream           ss_;                        // String Stream
    std::string               line_;                        // Buffer for line
    std::string                cmd_;                        // Buffer for command
    std::string                opt_;                        // Buffer for option 
    std::string                arg_;                        // Buffer for argument 
    
    void readLefCmd          ();                            // Wrapper for read_lef     in LefDefParser
    void readDefCmd          ();                            // Wrapper for read_def     in LefDefParser
    void readVerilogCmd      ();                            // Wrapper for read_verilog in LefDefParser
    void printInfoCmd        ();                            // Wrapper for printInfo    in LefDefParser

    // Table: [CMD String] [Function Pointer]
    // Inspired by OpenTimer...
    std::unordered_map<std::string, void(CmdInterpreter::*)()> cmdList_ 
    {
      {"read_lef"    ,   &CmdInterpreter::readLefCmd     },
      {"read_def"    ,   &CmdInterpreter::readDefCmd     },
      {"read_verilog",   &CmdInterpreter::readVerilogCmd },
      {"print_info"  ,   &CmdInterpreter::printInfoCmd   }
    };
};
