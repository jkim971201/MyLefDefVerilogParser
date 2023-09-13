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

std::vector<std::string> tokenize(const std::filesystem::path& path, 
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

void readDefOnePin(std::vector<std::string>::iterator& itr)
{
  std::string pinName;
  std::string netName;

  std::string pinStatus;
  std::string pinDirection;
  std::string pinOrient;
  std::string pinLayer;

  int offsetLx = 0;
  int offsetLy = 0;

  int offsetUx = 0;
  int offsetUy = 0;

  int originX = 0;
  int originY = 0;

  pinName = std::move( *(++itr) );

  assert( *(++itr) == "+" );

  netName = std::move( *(++itr) );

  while( *(itr + 1) != "END" && *(itr + 1) != "-" ) 
  {
    if( *itr == "DIRECTION" )
      pinDirection = std::move( *(++itr) );

    else if( *itr == "FIXED" || *itr == "PLACED" )
    {
      pinStatus = std::move( *(itr) );

      assert(*(++itr) == "(");

      originX = std::stoi( *(++itr) );

      originY = std::stoi( *(++itr) );

      assert(*(++itr) == ")");

      pinOrient = std::move( *(++itr) );
    }

    else if( *itr == "LAYER" )
    {
      pinLayer = std::move( *(++itr) );

      assert(*(++itr) == "(");

      offsetLx = std::stoi( *(++itr) );

      offsetLy = std::stoi( *(++itr) );

      assert(*(++itr) == ")");

      assert(*(++itr) == "(");

      offsetUx = std::stoi( *(++itr) );

      offsetUy = std::stoi( *(++itr) );

      assert(*(++itr) == ")");
    }
    else
      itr++;
  }

  std::cout << "PinName : " << pinName;

  std::cout << " NetName : " << netName;

  std::cout << " PinStatus : " << pinStatus << std::endl;
}

void readDefPins(std::vector<std::string>::iterator& itr,
                 const std::vector<std::string>::iterator& end,
                 int& numPin)
{
  numPin = std::stoi( *(++itr) );

  while(++itr != end)
  {
    if(*itr == "-")
      readDefOnePin(itr);
    else if(*itr == "END" && *(++itr) == "PINS")
      break;
    else
    {
      std::cout << "Syntax Error while Reading DEF PINS" << std::endl;
      std::cout << *(itr) << std::endl;
      exit(0);
    }
  }
}

int main(int argc, char** argv)
{
  if(argc < 4)
  {
    std::cout << "Please give input file" << std::endl;
    exit(0);
  }

  std::filesystem::path fileName  = argv[1];

  std::filesystem::path fileName2 = argv[2];

  std::filesystem::path fileName3 = argv[3];

  //readVerilog(fileName);

  //readLef(fileName);
  
  //readDef(fileName);
  
  LefDefDB::LefDefParser parser;

  parser.readLef(fileName);

  parser.readVerilog(fileName2);

	parser.readDef(fileName3);

  return 0;
}
