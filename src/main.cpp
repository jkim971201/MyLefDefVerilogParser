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
#include "Painter.h"

using namespace LefDefDB;
using namespace Graphic;

int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cout << "Please give input cmd file" << std::endl;
    exit(0);
  }

  std::filesystem::path cmdfile = argv[1];

  std::shared_ptr<LefDefParser> parser;
  parser = std::make_shared<LefDefParser>();

  std::shared_ptr<Painter> painter;
  painter = std::make_shared<Painter>(parser);

  CmdInterpreter cmd;
  cmd.setParser(parser);
  cmd.setPainter(painter);

  cmd.readCmd(cmdfile);

  return 0;
}
