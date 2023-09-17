#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include "LefDefParser.h"

using namespace LefDefDB;

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
    std::ifstream                   file_;                  // Input File Stream
    std::stringstream                 ss_;                  // String Stream
    std::string                     line_;                  // Buffer for line
    std::string                      cmd_;                  // Buffer for command
    std::string                      arg_;                  // Buffer for argument 

    void readLefCmd          ();
    void readDefCmd          ();
    void readVerilogCmd      ();
    void printInfoCmd        ();

    std::unordered_map<std::string, void(CmdInterpreter::*)()> cmdList_ 
    {
      {"read_lef"    ,   &CmdInterpreter::readLefCmd     },
      {"read_def"    ,   &CmdInterpreter::readDefCmd     },
      {"read_verilog",   &CmdInterpreter::readVerilogCmd },
      {"print_info"  ,   &CmdInterpreter::printInfoCmd   }
    };
};
