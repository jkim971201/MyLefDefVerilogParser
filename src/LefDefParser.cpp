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

  for(size_t i = 0; i < fsize; ++i) 
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
  : numPI_             (0),
    numPO_             (0),
    numInst_           (0),
    numNet_            (0),
    numPin_            (0),
    numRow_            (0),
    numDummy_          (0),
		numDefComponents_  (0)
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

  strToCellOrient_["N" ] = CellOrient::N;
  strToCellOrient_["S" ] = CellOrient::S;
  strToCellOrient_["FN"] = CellOrient::FN;
  strToCellOrient_["FS"] = CellOrient::FS;
}

void 
LefDefParser::readLefPinShape(strIter& itr, const strIter& end, LefPin* lefPin)
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
      readLefPinShape(itr, end, &lefPin);
  
    else if(*itr == "END" && *(itr + 1) == pinName)
      break;
  }

	PinUsage     pUsage;
	PinDirection pDirection;

	auto pinUsageCheck      = strToPinUsage_.find(pinUsage);
	auto pinDirectionCheck  = strToPinDirection_.find(pinDirection);

	if(pinUsageCheck == strToPinUsage_.end())
	{
		std::cout << "Error - PIN USAGE " << pinUsage;
		std::cout << " is not supported yet." << std::endl;
		exit(0);
	}
	else
		pUsage = pinUsageCheck->second;

	if(pinDirectionCheck == strToPinDirection_.end())
	{
		std::cout << "Error - PIN DIRECTION " << pDirection;
		std::cout << " is not supported yet." << std::endl;
		exit(0);
	}
	else
		pDirection = pinDirectionCheck->second;

  lefPin.setPinUsage( pUsage );
  lefPin.setPinDirection( pDirection );

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

	MacroClass mcClass;
	LefSite* lefSite;

	auto classCheck = strToMacroClass_.find(macroClass);
	auto siteCheck  = siteMap_.find(siteName);

	if(classCheck == strToMacroClass_.end())
	{
		std::cout << "Error - CLASS " << macroClass;
		std::cout << " is not supported yet." << std::endl;
		exit(0);
	}
	else
		mcClass = classCheck->second;
	
	if(siteCheck == siteMap_.end())
	{
		if(macroClass != "BLOCK") // BLOCK MACRO does not have SITE
		{
			std::cout << "Error - SITE " << siteName;
			std::cout << " is not found in the LEF." << std::endl;
			exit(0);
		}
	}
	else
		lefSite = siteCheck->second;

  lefMacro.setClass( mcClass );
  lefMacro.setSite ( lefSite );

  lefMacro.setSizeX(sizeX);
  lefMacro.setSizeY(sizeY);

  lefMacro.setOrigX(origX);
  lefMacro.setOrigY(origY);

  macros_.push_back(lefMacro);

  if(itr == end)
  {
    std::cout << "Syntax Error in LEF."     << std::endl;
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

	SiteClass sClass;
	auto siteClassCheck = strToSiteClass_.find(siteClass);

	if(siteClassCheck == strToSiteClass_.end())
	{
		std::cout << "Error - SITE CLASS " << siteClass;
		std::cout << " is not supported yet." << std::endl;
		exit(0);
	}
	else
		sClass = siteClassCheck->second;

  LefSite lefSite(siteName, sClass, sizeX, sizeY);

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
  
  auto tokens = tokenize(path, delimiters, exceptions);

  auto itr = tokens.begin();
  auto end = tokens.end();

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
      designName_ = *itr;
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
			{
        numPI_++;
			}
    }
    else if(*itr == "output") 
    {
      while(++itr != end && *itr != ";") 
			{
        numPO_++;
			}
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
      std::string macroName = std::move(*itr);

      auto checkMacro = macroMap_.find(macroName);

      LefMacro* lefMacro;

      if(checkMacro == macroMap_.end() )
      {
        std::cout << "Error - Macro " << macroName << " ";
        std::cout << "is not found in the LEF." << std::endl;
        exit(0);
      }
      else
        lefMacro = checkMacro->second;

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

      if(numInst_ % 200000 == 0)
      {
        using namespace std;
        cout << "Read ";
        cout << setw(7) << right << numInst_ << " Instances..." << endl;
      }
    }
  }

  dbCellPtrs_.reserve(numInst_);
  dbPinPtrs_.reserve(numPin_);
  dbNetPtrs_.reserve(numNet_);

  // Make Pointer Vector
  for(auto& cell : dbCellInsts_)
    dbCellPtrs_.push_back(&cell);

  // Make Pointer Vector
  for(auto& net : dbNetInsts_)
    dbNetPtrs_.push_back(&net);

  // Make Pointer Vector & Add Interconnect Information
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
  std::cout << "| Module Name : " << designName_ << std::endl;
  std::cout << "| Num PI      : " << numPI_      << std::endl;
  std::cout << "| Num PO      : " << numPO_      << std::endl;
  std::cout << "| Num Inst    : " << numInst_    << std::endl;
  std::cout << "| Num Net     : " << numNet_     << std::endl;
  std::cout << "| Num Pin     : " << numPin_     << std::endl;
  std::cout << "=================================="  << std::endl;
}

void
LefDefParser::readDefRow(strIter& itr, const strIter& end)
{
  std::string rowName;
  std::string siteName;

  std::string siteOrient;

  int origX = 0;
  int origY = 0;

  int numSiteX = 0;
  int numSiteY = 0;

  int stepX = 0;
  int stepY = 0;

  rowName  = std::move( *(++itr) );
  siteName = std::move( *(++itr) );
    
  origX = std::stoi( *(++itr) );
  origY = std::stoi( *(++itr) );

  siteOrient = std::move( *(++itr) );

  assert( *(++itr) == "DO" );

  numSiteX = std::stoi( *(++itr) );

  assert( *(++itr) == "BY" );

  numSiteY = std::stoi( *(++itr) );

  assert( *(++itr) == "STEP" );

  stepX = std::stoi( *(++itr) );

  stepY = std::stoi( *(++itr) );

  auto checkSite = siteMap_.find(siteName);

  LefSite* lefSite;

  if(checkSite == siteMap_.end() )
  {
    std::cout << "Error - Site " << siteName;
    std::cout << " is not found in the LEF." << std::endl;
    exit(0);
  }
  else
    lefSite = checkSite->second;

  dbRow row(rowName, lefSite, 
            dbUnit_,
            origX, origY, 
            numSiteX, numSiteY,
            stepX, stepY);

  dbRowInsts_.push_back(row);

  numRow_++;
}

void 
LefDefParser::readDefDie(strIter& itr, const strIter& end)
{
  int lx = 0;
  int ly = 0;

  int ux = 0;
  int uy = 0;

  while(++itr != end)
  {
    if( *(itr) == "(" )
    {
      lx = std::stoi(*(++itr));
      ly = std::stoi(*(++itr));
      assert( *(++itr) == ")" );
      assert( *(++itr) == "(" );
      ux = std::stoi(*(++itr));
      uy = std::stoi(*(++itr));
      assert( *(++itr) == ")" );
      break;
    }
  }

  if(itr == end)
  {
    std::cout << "Syntax Error in DEF." << std::endl;
    exit(0);
  }

  die_.setCoordi(lx, ly, ux, uy);
}

void
LefDefParser::readDefOneComponent(strIter& itr, const strIter& end)
{
  std::string instName;
  std::string macroName;

  std::string cellStatus;

  LefMacro* lefMacro;

  int coordiX = 0;
  int coordiY = 0;

  std::string cellOrient;

  instName = std::move( *(++itr) );

  macroName = std::move( *(++itr) );

  assert( *(++itr) == "+" );

  cellStatus = std::move( *(++itr) );

  assert( *(++itr) == "(" );

  coordiX = std::stoi( *(++itr));

  coordiY = std::stoi( *(++itr));

  assert( *(++itr) == ")" );

  cellOrient = std::move( *(++itr) );

  int cellID;
  auto checkCell  = strToCellID_.find(instName);
  auto checkMacro = macroMap_.find(macroName);

  bool isFixed = (cellStatus == "FIXED") ? true : false;

  if(checkMacro == macroMap_.end() )
  {
    std::cout << "Error - Macro " << macroName << " ";
    std::cout << "is not found in the LEF." << std::endl;
    exit(0);
  }
  else
    lefMacro = checkMacro->second;

  dbCell* cell;

  if(checkCell == strToCellID_.end())
  {
    // These components do not exist in the Verilog file
    // but they do exist in the DEF file
    // I don't know why...
    cellID = numInst_;

    dbCell newCell(cellID, instName, lefMacro);

    dbCellInsts_.push_back(newCell);

    cell = &newCell;

    numInst_++;
    numDummy_++;
  }
  else
  {
    cellID = checkCell->second;
    cell   = dbCellPtrs_[cellID];
  }


	CellOrient orient;
	auto orientCheck = strToCellOrient_.find(cellOrient);

	if(orientCheck == strToCellOrient_.end())
	{
		std::cout << "Error - ORIENT " << cellOrient;
		std::cout << " is not supported yet." << std::endl;
		exit(0);
	}
	else
		orient = orientCheck->second;

  cell->setCellOrient(orient);

  cell->setLx(coordiX);
  cell->setLy(coordiY);

	numDefComponents_++;

	if(numDefComponents_ % 200000 == 0)
	{
		using namespace std;
		cout << "Read " << setw(7) << numDefComponents_ << " Components..." << endl;
	}
}

void
LefDefParser::readDefComponents(strIter& itr, const strIter& end)
{
	int defComponents = std::stoi( std::move( *(++itr) ) );

  while(++itr != end)
  {
    if(*itr == "-")
      readDefOneComponent(itr, end);
    else if(*itr == "END" && *(++itr) == "COMPONENTS")
      break;
    else
    {
      std::cout << "Syntax Error while Reading DEF COMPONENTS" << std::endl;
      exit(0);
    }
  }
}

void 
LefDefParser::readDef(const std::filesystem::path& fileName)
{
  std::cout << "readDef: " << fileName << std::endl;

  static std::string_view delimiters = "#;";
  static std::string_view exceptions = "";
  
  auto tokens = tokenize(fileName, delimiters, exceptions);

  auto itr = tokens.begin();
  auto end = tokens.end();

  std::string designName;

  int numComponent = 0;
  int numPin       = 0;
  int numRow       = 0;
  int numNet       = 0;

  while(++itr != end) 
  {
    if(*itr == "DESIGN")
      designName = std::move( *(++itr) );
    else if(*itr == "DIEAREA")
      readDefDie(itr, end);
    else if(*itr == "ROW")
      readDefRow(itr, end);
    else if(*itr == "COMPONENTS")
      readDefComponents(itr, end);
//    else if(*itr == "PINS")
//      readDefPins(itr, end, numPin);
    else if(*itr == "END" && *(itr + 1) == "DESIGN")
      break;
  }

  // Make Row Ptrs
  for(auto& r : dbRowInsts_)
    dbRowPtrs_.push_back(&r);

  std::cout << "=================================="  << std::endl;
  std::cout << "  DEF Statistic                   "  << std::endl;
  std::cout << "=================================="  << std::endl;
  std::cout << "| Design Name    : " << designName   << std::endl;
  std::cout << "| Num ROWS       : " << numRow       << std::endl;
  std::cout << "| Num COMPONENTS : " << numComponent << std::endl;
  std::cout << "| Num PINS       : " << numPin       << std::endl;
  std::cout << "| Num NETS       : " << numNet       << std::endl;
  std::cout << "=================================="  << std::endl;
}

};
