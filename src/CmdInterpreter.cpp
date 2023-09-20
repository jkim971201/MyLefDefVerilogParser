#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "CmdInterpreter.h"

inline void argumentError(const std::string& cmd)
{
  std::cout << "Please give argument to " << cmd << "\n";
  exit(0);
}

inline void optionError(const std::string& opt, const std::string& cmd)
{
  std::cout << "Unknown option " << opt << "for " << cmd << "\n";
  exit(0);
}

inline void typeError(const std::string& type)
{
  std::cout << "Please give ." << type << " file...\n";
  exit(0);
}

inline bool checkFileType(const std::filesystem::path& path, const std::string& type)
{
  std::string filename = std::string(path);
  size_t dot = filename.find_last_of('.');
  std::string filetype = filename.substr(dot + 1);

  if(filetype != type)
    return false;
  else
    return true;
}

CmdInterpreter::CmdInterpreter()
  : parser_   (nullptr),
    painter_  (nullptr)
{}

void
CmdInterpreter::readCmd(const std::filesystem::path& cmdfile)
{
  if(parser_ == nullptr || painter_ == nullptr)
  {
    std::cout << "Submodule for CmdInterpreter is not set!" << std::endl;
    exit(0);
  }

  std::cout << "Read " << cmdfile << std::endl;

  if( !checkFileType(cmdfile, "cmd") )
    typeError("cmd");

  std::string filename = std::string(cmdfile);
 
  file_.open(cmdfile);

  bool quit = false;

  while(!file_.eof() && !quit)
  {
    line_.clear();
    cmd_.clear();
    opt_.clear();
    arg_.clear();

    std::getline(file_, line_);

    if(auto comment = line_.find('#'); comment != std::string::npos)
      line_.erase(comment);
    // If # is found, delete the rest of the line

    if(line_.empty()) 
      continue;

    ss_ = std::stringstream(line_);
    ss_ >> cmd_;

    if(auto findCmd = cmdList_.find(cmd_); findCmd != cmdList_.end())
      (this->*(findCmd->second))();
    else
    {
      std::cout << "Undefined Command: " << cmd_ << std::endl;
      quit = true;
    }
  }
}

void
CmdInterpreter::readLefCmd()
{
  ss_ >> opt_;

  if(opt_ == "-dir")
  {
    ss_ >> arg_;
    
    for(auto& file : dirItr(arg_) )
    {
      if( !checkFileType(file.path(), "lef") )
        continue;
      parser_->readLef( file.path() );
    }
  }
  else
  {
    arg_ = opt_;

    if(arg_.empty())
      argumentError(cmd_);
    else if(arg_[0] == '-')
      optionError(opt_, cmd_);
    else
      parser_->readLef(arg_);
  }
}

void
CmdInterpreter::readDefCmd()
{
  ss_ >> arg_;

  if(arg_.empty())
    argumentError(cmd_);
  else
    parser_->readDef(arg_);
}

void
CmdInterpreter::readVerilogCmd()
{
  ss_ >> arg_;

  if(arg_.empty())
    argumentError(cmd_);
  else
    parser_->readVerilog(arg_);
}

void
CmdInterpreter::printInfoCmd()
{
  parser_->printInfo();
}

void
CmdInterpreter::drawChipCmd()
{
  painter_->drawChip();
}
