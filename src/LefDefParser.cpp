#include <iostream>
#include <iomanip>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cfloat>

#include "LefDefParser.h"

namespace LefDefDB
{

// Function: on_next_parentheses
// Iteratively apply closure c on each token inside the parentheses pair
// I : Iterator C: Closure 
template <typename I, typename C>
auto on_next_parentheses(const I start, const I end, C&& c) 
{
  auto left  = std::find(start, end, "(");
  auto right = left;

  int stack = 0;

  // This while loop is necessary for
  // detecting the inner parentheses pairs
  while(right != end) 
  {
    if(*right == "(") 
      ++stack;
    else if(*right == ")")
      --stack;
    if(stack == 0)
      break;
    ++right;
  }
  
  if(left == end || right == end) 
    return end;
  else
    for(++left; left != right; ++left) { c(*left); }

  return right;
}


// LEF-related
void 
LefMacro::printInfo() const
{
  using namespace std;

  cout << "----------------------------------------" << endl;
  cout << "MACRO : " << setw(10) << left         << macroName_ << endl;;
  cout << "SIZEX : " << setw(4 ) << setfill(' ') << sizeX_     << endl;
  cout << "SIZEY : " << setw(4 ) << setfill(' ') << sizeY_     << endl;
  cout << "PIN   : " << setw(3 ) << setfill(' ') << left       << pins_.size();

  int numPin = 0;
  int numPinForOneLine = 8;

  for(auto & pin : pins_)
  {
    if(numPin++ % numPinForOneLine == 0)
      cout << endl;

    if(pin.usage() == PinUsage::SIGNAL)
      cout << setw(4) << left << pin.name() << " ";
  }

  cout << endl;
  cout << "----------------------------------------" << endl;
}

std::pair<float, float>
LefPin::getPinCoordi() const
{
  float minX = FLT_MAX;
  float minY = FLT_MAX;
  float maxX = FLT_MIN;
  float maxY = FLT_MIN;

  for(auto& r : lefRect_)
  {
    float lx = r.lx;
    float ly = r.ly;
    float ux = r.ux;
    float uy = r.uy;

    if(minX < lx)
      minX = lx;
    if(minY < ly)
      minY = ly;
    if(maxX < ux)
      maxX = ux;
    if(maxY < uy)
      maxY = uy;
  }

  float cX = (minX + maxX) / 2.0;
  float cY = (minY + maxY) / 2.0;

  return std::pair<float, float>(cX, cY);
}

std::vector<std::string>
LefDefParser::tokenize(const std::filesystem::path& path, 
                       std::string_view dels,
                       std::string_view exps) 
{
  using namespace std::literals::string_literals;

  std::ifstream ifs(path, std::ios::ate);

  if(!ifs.good()) 
  {
    throw std::invalid_argument("failed to open the file '"s + path.c_str() + '\'');
    return {};
  }
  
  // Read the file to a local buffer.
  size_t fsize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::vector<char> buffer(fsize + 1);
  ifs.read(buffer.data(), fsize);
  buffer[fsize] = 0;
  
  // Mart out the comment
  for(size_t i=0; i<fsize; ++i) 
  {
    // Block comment
    if(buffer[i] == '/' && buffer[i+1] == '*') 
    {
      buffer[i] = buffer[i+1] = ' ';

      for(i=i+2; i<fsize; buffer[i++]=' ') 
      {
        if(buffer[i] == '*' && buffer[i+1] == '/') 
        {
          buffer[i] = buffer[i+1] = ' ';
          i = i+1;
          break;
        }
      }
    }
    
    // Line comment
    if(buffer[i] == '/' && buffer[i+1] == '/') 
    {
      buffer[i] = buffer[i+1] = ' ';

      for(i=i+2; i<fsize; ++i) 
      {
        if(buffer[i] == '\n' || buffer[i] == '\r') 
          break;
        else 
          buffer[i] = ' ';
      }
    }
    
    // Pond comment
    if(buffer[i] == '#') 
    {
      buffer[i] = ' ';

      for(i=i+1; i<fsize; ++i) 
      {
        if(buffer[i] == '\n' || buffer[i] == '\r') 
          break;
        else 
          buffer[i] = ' ';
      }
    }
  }

  //std::cout << std::string_view(buffer.data()) << std::endl;

  // Parse the token.
  std::string token;
  std::vector<std::string> tokens;

  for(size_t i=0; i<fsize; ++i) 
  {
    auto c = buffer[i];
    bool is_del = (dels.find(c) != std::string_view::npos);

    if(is_del || std::isspace(c)) 
    {
      if(!token.empty()) 
      {                            
        // Add the current token.
        tokens.push_back(std::move(token));
        token.clear();
      }
      if(is_del && exps.find(c) != std::string_view::npos) 
      {
        token.push_back(c);
        tokens.push_back(std::move(token));
      }
    } 
    else 
      token.push_back(c);  // Add the char to the current token.
  }

  if(!token.empty()) 
    tokens.push_back(std::move(token));

  return tokens;
}

LefDefParser::LefDefParser()
  : numPI_      (0),
    numPO_      (0),
    numInst_    (0),
    numNet_     (0),
    numPin_     (0)
{
  strToMacroClass_["CORE"       ] = MacroClass::CORE;
  strToMacroClass_["CORE_SPACER"] = MacroClass::CORE_SPACER;
  strToMacroClass_["PAD"        ] = MacroClass::PAD;
  strToMacroClass_["BLOCK"      ] = MacroClass::BLOCK;
  strToMacroClass_["ENDCAP"     ] = MacroClass::ENDCAP;
  
  strToSiteClass_["CORE"] = SiteClass::CORE_SITE;

  strToPinDirection_["INPUT" ] = PinDirection::INPUT;
  strToPinDirection_["OUTPUT"] = PinDirection::OUTPUT;
  strToPinDirection_["INOUT" ] = PinDirection::INOUT;

  strToPinUsage_["SIGNAL"] = PinUsage::SIGNAL;
  strToPinUsage_["POWER" ] = PinUsage::POWER;
}

void 
LefDefParser::readLefPort(strIter& itr, const strIter& end, LefPin* lefPin)
{
  std::string portName;
  std::string layerName;

  portName = *(++itr);

  while(++itr != end)
  {
    if(*itr == "LAYER")
      layerName = std::move(*(++itr));

    else if(*itr == "RECT")
    {
      float lx, ly, ux, uy = 0.0;
      lx = std::stof(*(++itr));
      ly = std::stof(*(++itr));
      ux = std::stof(*(++itr));
      uy = std::stof(*(++itr));

      LefRect rect(lx, ly, ux, uy);

      lefPin->addLefRect(rect);
    }
    else if(*itr == "END")
      break;
  }

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF." << std::endl;
    exit(0);
  }
}

void 
LefDefParser::readLefPin(strIter& itr, const strIter& end, LefMacro* lefMacro)
{
  float sizeX = 0.0;
  float sizeY = 0.0;

  std::string pinName;
  std::string pinDirection = "INPUT";
  std::string pinUsage = "SIGNAL";

  pinName = *(++itr);

  LefPin lefPin(pinName, lefMacro);

  while(++itr != end)
  {
    if(*itr == "DIRECTION")
      pinDirection = *(++itr);

    if(*itr == "USE")
      pinUsage = *(++itr);

    else if(*itr == "PORT")
      readLefPort(itr, end, &lefPin);
  
    else if(*itr == "END" && *(itr + 1) == pinName)
      break;
  }

  lefPin.setPinUsage(strToPinUsage_[pinUsage]);
  lefPin.setPinDirection(strToPinDirection_[pinDirection]);

  lefMacro->addPin(lefPin);

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF." << std::endl;
    std::cout << "No END keyword in PIN " << pinName << std::endl;
    exit(0);
  }
}

void 
LefDefParser::readLefMacro(strIter& itr, const strIter& end)
{
  std::string macroName = *(++itr);
  std::string macroClass;
  std::string siteName;

  float origX = 0.0;
  float origY = 0.0;

  float sizeX = 0.0;
  float sizeY = 0.0;

  LefMacro lefMacro(macroName);

  while(++itr != end)
  {
    if(*itr == "CLASS")
      macroClass = std::move( *(++itr) );

    else if(*itr == "ORIGIN")
    {
      origX = std::stof( *(++itr) );
      origY = std::stof( *(++itr) );
    }

    else if(*itr == "SITE")
      siteName = std::move( *(++itr) );

    else if(*itr == "SIZE")
    {
      sizeX = std::stof( *(++itr) );
      assert(*(++itr) == "BY");
      sizeY = std::stof( *(++itr) );
    }

    else if(*itr == "PIN")
      readLefPin(itr, end, &lefMacro);

    else if(*itr == "END" && *(itr + 1) == macroName)
      break;
  }

  lefMacro.setClass( strToMacroClass_[macroClass] );
  lefMacro.setSite ( siteMap_[siteName] );

  lefMacro.setSizeX(sizeX);
  lefMacro.setSizeY(sizeY);

  lefMacro.setOrigX(origX);
  lefMacro.setOrigY(origY);

  macros_.push_back(lefMacro);

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF." << std::endl;
    std::cout << "No END keyword in MACRO " << macroName << std::endl;
    exit(0);
  }
}

void 
LefDefParser::readLefSite(strIter& itr, const strIter& end)
{
  float sizeX = 0.0;
  float sizeY = 0.0;

  std::string siteName;
  std::string siteClass;

  siteName = *(++itr);

  while(++itr != end)
  {
    //std::cout << *itr << std::endl;
    if(*itr == "CLASS")
      siteClass = *(++itr);

    else if(*itr == "SIZE")
    {
      sizeX = std::stof(*(++itr));
      assert(*(++itr) == "BY");
      sizeY = std::stof(*(++itr));
    }

    if(*itr == "END" && *(itr + 1) == siteName)
      break;
  }

  LefSite lefSite(siteName, 
                  strToSiteClass_[siteClass],
                  sizeX, sizeY);

  sites_.push_back(lefSite);

  siteMap_[siteName] = &(*sites_.end());

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF." << std::endl;
    std::cout << "No END keyword in SITE " << siteName << std::endl;
    exit(0);
  }
}

void 
LefDefParser::readLefUnit(strIter& itr, const strIter& end)
{
  while(++itr != end)
  {
    if(*itr == "DATABASE")
    {
      assert(*(++itr) == "MICRONS");
      dbUnit_ = std::stoi(*(++itr));
    }
    else if(*itr == "END" && *(++itr) == "UNITS")
      break;
  }

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF." << std::endl;
    std::cout << "No END keyword in UNITS" << std::endl;
    exit(0);
  }
}

void 
LefDefParser::readLef(const std::filesystem::path& fileName)
{
  std::cout << "readLef : " << fileName << std::endl;

  static std::string_view delimiters = "#;";
  static std::string_view exceptions = "";
  
  auto tokens = tokenize(fileName, delimiters, exceptions);

  auto itr = tokens.begin();
  auto end = tokens.end();

  while(++itr != end) 
  {
    if(*itr == "SITE")
      readLefSite(itr, end);
    else if(*itr == "UNITS")
      readLefUnit(itr, end);
    else if(*itr == "MACRO")
      readLefMacro(itr, end);
    else if(*itr == "END" && *(itr + 1) == "LIBRARY")
      break;
  }

  for(auto& macro : macros_)
    macroMap_[macro.name()] = &macro;

  printLefStatistic();
}

void 
LefDefParser::printLefStatistic() const
{
  int numLefMacro = macros_.size();
  int numLefSite  = sites_.size();

  for(auto& macro : macros_)
    macro.printInfo();

  std::cout << "=================================="  << std::endl;
  std::cout << "  LEF Statistic                   "  << std::endl;
  std::cout << "=================================="  << std::endl;
  std::cout << "| Num LefMacro : " << numLefMacro << std::endl;
  std::cout << "| Num LefUnit  : " << dbUnit_     << std::endl;
  std::cout << "| Num LefSite  : " << numLefSite  << std::endl;
  std::cout << "=================================="  << std::endl;
}

// Verilog-related
void
LefDefParser::readVerilog(const std::filesystem::path& path)
{
  std::cout << "readVerilog : " << path << std::endl;

  static std::string_view delimiters = "(),:;/#[]{}*\"\\";
  static std::string_view exceptions = "().;";
  //static std::string_view exceptions = "";
  
  auto tokens = tokenize(path, delimiters, exceptions);

  auto itr = tokens.begin();
  auto end = tokens.end();

  std::string moduleName;

  // Read the module name
  itr = std::find(itr, end, "module");

  if(itr == end) 
  {
    std::cout << "No module keyworkd in the .v file." << std::endl;
    exit(0);
  }
  else 
  {
    if(++itr == end) 
    {
      std::cout << "Module name is invalid." << std::endl;
      exit(0);
    }
    else
      moduleName = *itr;
  }

  while(++itr != end && *itr != ";") 
  {
    if(*itr != "(" && *itr != ")") 
    {
      //std::cout << "Port Name: " << *itr << std::endl;
    }
  }

  // Parse the content.
  while(++itr != end) 
  {
    if(*itr == "endmodule") 
      break;
    else if(*itr == "input") 
    {
      while(++itr != end && *itr != ";") 
        numPI_++;
    }
    else if(*itr == "output") 
    {
      while(++itr != end && *itr != ";") 
        numPO_++;
    }
    else if(*itr == "wire") 
    {
      while(++itr != end && *itr != ";") 
      {
        int netID = numNet_;
        std::string netName = std::move( *(itr) );
        dbNet net(netID, netName);
        dbNetInsts_.push_back(net);

        strToNetID_[netName] = netID;

        numNet_++;
      }
    }
    else 
    {
      LefMacro* lefMacro = macroMap_[std::move(*itr)];

      if(++itr == end) 
      {
        std::cout << "Syntax error while reading Verilog." << std::endl;
        exit(0);
      }

      int cellID = numInst_;

      std::string cellName = std::move(*(itr));

      dbCell cell(cellID, cellName, lefMacro);

      strToCellID_[cellName] = cellID;

      std::string portName;
      std::string netName;

      itr = on_next_parentheses(itr, end, [&] (auto& str) mutable { 
        if(str == ")" || str == "(") 
          return;
        else if(str[0] == '.') 
          portName = std::move( str.substr(1) );
        else 
        {
          netName   = std::move( str );
					int netID = strToNetID_[netName];
					int pinID = numPin_;

					std::string pinName = portName + ":" + cellName;
					dbPin pin(pinID, cellID, netID, pinName, lefMacro->getPin(portName));

					dbPinInsts_.push_back(pin);
          numPin_++;
        }
      });

      if(itr == end) 
        std::cout << "Syntax error in gate pin-net mapping" << std::endl;

      if(*(++itr) != ";") 
        std::cout << "Missing ; in instance declaration" << std::endl;

			dbCellInsts_.push_back(cell);
      numInst_++;
    }
  }

  dbCellPtrs_.reserve(numInst_);
  dbPinPtrs_.reserve(numPin_);
  dbNetPtrs_.reserve(numNet_);

	for(auto& cell : dbCellInsts_)
		dbCellPtrs_.push_back(&cell);

	for(auto& net : dbNetInsts_)
		dbNetPtrs_.push_back(&net);

	for(auto& pin : dbPinInsts_)
	{
		int cellID = pin.cid();
		int netID  = pin.nid();

		dbCell* cellPtr = &( dbCellInsts_[cellID] );
		dbNet*  netPtr  = &( dbNetInsts_[netID]   );

		cellPtr->addPin(&pin);
		netPtr->addPin(&pin);

		pin.setCell( cellPtr ); 
		pin.setNet( netPtr );

		dbPinPtrs_.push_back(&pin);
	}

  std::cout << "=================================="  << std::endl;
  std::cout << "  Netlist (.v) Statistic          "  << std::endl;
  std::cout << "=================================="  << std::endl;
  std::cout << "| Module Name : " << moduleName << std::endl;
  std::cout << "| Num PI      : " << numPI_     << std::endl;
  std::cout << "| Num PO      : " << numPO_     << std::endl;
  std::cout << "| Num Inst    : " << numInst_   << std::endl;
  std::cout << "| Num Net     : " << numNet_    << std::endl;
  std::cout << "| Num Pin     : " << numPin_    << std::endl;
  std::cout << "=================================="  << std::endl;
}

};
