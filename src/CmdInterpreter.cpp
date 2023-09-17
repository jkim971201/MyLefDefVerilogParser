#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "CmdInterpreter.h"

inline void argumentError(std::string& cmd)
{
	std::cout << "Please give argument to " << cmd << std::endl;
	exit(0);
}

void
CmdInterpreter::readCmd(const std::filesystem::path& cmdfile)
{
  std::cout << "Read " << cmdfile << std::endl;

	std::string filename = std::string(cmdfile);

  size_t dot = filename.find_last_of('.');
	std::string filetype = filename.substr(dot + 1);

	if(filetype != "cmd")
	{
		std::cout << "Please give .cmd file" << std::endl;
		exit(0);
	}

  file_.open(cmdfile);

	bool quit = false;

  while(!file_.eof() && !quit)
  {
		line_.clear();
    cmd_.clear();
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
	ss_ >> arg_;
	
	if(arg_.empty())
		argumentError(cmd_);
	else
		parser_->readLef(arg_);
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
