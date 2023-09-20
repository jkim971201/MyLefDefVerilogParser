#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include "LefDefParser.h"
#include "Painter.h"

using namespace LefDefDB;
using namespace Graphic;

typedef std::filesystem::directory_iterator dirItr;

class CmdInterpreter
{
  public:
    
    CmdInterpreter();                                       // Constructor

		// Setters
		void setParser   (std::shared_ptr<LefDefParser> parser )  { parser_   = parser;   }
		void setPainter  (std::shared_ptr<Painter>      painter)  { painter_  = painter;  }

		// For main functionality
    void readCmd(const std::filesystem::path& cmdfile);     // Read Command (.cmd) file

  private:

    std::shared_ptr<LefDefParser> parser_;                  // SharedPointer of Parser
    std::shared_ptr<Painter>      painter_;                 // SharedPointer of Painter

    // For parsing .cmd file
    std::ifstream         file_;                            // Input File Stream
    std::stringstream       ss_;                            // String Stream
    std::string           line_;                            // Buffer for line
    std::string            cmd_;                            // Buffer for command
    std::string            opt_;                            // Buffer for option 
    std::string            arg_;                            // Buffer for argument 
    
    void readLefCmd          ();                            // Wrapper for read_lef     in LefDefParser
    void readDefCmd          ();                            // Wrapper for read_def     in LefDefParser
    void readVerilogCmd      ();                            // Wrapper for read_verilog in LefDefParser
    void printInfoCmd        ();                            // Wrapper for printInfo    in LefDefParser
    
		void drawChipCmd         ();                            // Wrapper for drawChip     in Painter 

    // Table: [CMD String] [Function Pointer]
    // Inspired by OpenTimer...
    std::unordered_map<std::string, void(CmdInterpreter::*)()> cmdList_ 
    {
      {"read_lef"    ,   &CmdInterpreter::readLefCmd     },
      {"read_def"    ,   &CmdInterpreter::readDefCmd     },
      {"read_verilog",   &CmdInterpreter::readVerilogCmd },
      {"print_info"  ,   &CmdInterpreter::printInfoCmd   },
      {"draw_chip"   ,   &CmdInterpreter::drawChipCmd    }
    };
};
