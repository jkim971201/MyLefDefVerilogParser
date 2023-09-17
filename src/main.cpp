#include <stdio.h>
#include <cassert> // For Debug-Mode
#include <cmath>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "LefDefParser.h"
#include "CmdInterpreter.h"

using namespace LefDefDB;

int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cout << "Please give input file" << std::endl;
    exit(0);
  }

//  std::filesystem::path fileName  = argv[1];
//
//  std::filesystem::path fileName2 = argv[2];
//
//  std::filesystem::path fileName3 = argv[3];
//
//  LefDefDB::LefDefParser parser;
//
//  parser.readLef(fileName);
//
//  parser.readVerilog(fileName2);
//
//  parser.readDef(fileName3);
//
//  parser.printInfo();

  std::filesystem::path cmdfile = argv[1];

  std::shared_ptr<LefDefParser> parser;
  parser = std::make_shared<LefDefParser>();

  CmdInterpreter cmd(parser);

  cmd.readCmd(cmdfile);

  return 0;
}
