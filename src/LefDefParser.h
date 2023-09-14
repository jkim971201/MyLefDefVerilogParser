#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <climits>

namespace LefDefDB
{

// For Parsing LEF
typedef std::vector<std::string>::iterator strIter;

enum MacroClass   {CORE, CORE_SPACER, PAD, BLOCK, ENDCAP};
enum SiteClass    {CORE_SITE};
enum PinDirection {INPUT, OUTPUT, INOUT};
enum PinUsage     {SIGNAL, POWER};
enum Orient       {N, S, FN, FS};  // W E FW FE is not supported

struct LefRect
{
  float lx;
  float ly;
  float ux;
  float uy;

  LefRect(float lx, float ly, float ux, float uy)
    : lx(lx), ly(ly), ux(ux), uy(uy) {}
};

class LefMacro;
class LefPin;
class LefSite;

class dbCell;
class dbPin;
class dbNet;

class verilogCell;

class LefPin
{
  public: 

    LefPin(std::string pinName, LefMacro* lefMacro)
      : pinName_  (pinName   ),
        lefMacro_ (lefMacro  )
    {}

    // Setters
    void setPinUsage    (PinUsage     pinUsage) { pinUsage_ = pinUsage; }
    void setPinDirection(PinDirection pinDir)   { pinDir_   = pinDir;   }

    void addLefRect(LefRect rect) { lefRect_.push_back(rect); }

    // Getters
		float                 lx() const { return lx_;         }
		float                 ly() const { return ly_;         }
		float                 ux() const { return ux_;         }
		float                 uy() const { return uy_;         }

    LefMacro*          macro() const { return lefMacro_;   }
    std::string         name() const { return pinName_;    }
    PinUsage           usage() const { return pinUsage_;   }
    PinDirection   direction() const { return pinDir_;     }

    const std::vector<LefRect>& lefRect() const { return lefRect_; }

    void computeBBox();

  private:

    LefMacro* lefMacro_;

    std::string  pinName_;
    PinUsage     pinUsage_;
    PinDirection pinDir_;

    std::vector<LefRect> lefRect_;

		float lx_;
		float ly_;
		float ux_;
		float uy_;
};

class LefSite
{
  public: 

    LefSite(std::string siteName,
            SiteClass   siteClass,
            float sizeX,
            float sizeY)
    : siteName_  (siteName   ),
      siteClass_ (siteClass  ),
      sizeX_     (sizeX      ),
      sizeY_     (sizeY      )
    {}

    std::string name() const { return siteName_; }

    float sizeX() const { return sizeX_; }
    float sizeY() const { return sizeY_; }

    SiteClass siteClass() const { return siteClass_; }

  private:

    std::string siteName_;
    SiteClass   siteClass_;

    float sizeX_;
    float sizeY_;
};

class LefMacro
{
  public:
    
    LefMacro(std::string macroName)
      : macroName_ (macroName  )
    {}

    // Setters
    void setClass(MacroClass macroClass)  { macroClass_ = macroClass; }
    void setSite (LefSite* lefSite)       { macroSite_  = lefSite;    }
    void setSizeX(float sizeX)            { sizeX_ = sizeX; }
    void setSizeY(float sizeY)            { sizeY_ = sizeY; }
    void setOrigX(float origX)            { origX_ = origX; }
    void setOrigY(float origY)            { origY_ = origY; }

    void addPin(LefPin pin)               
    { 
      pins_.push_back(pin); 
      const LefPin* pinPtr = &(pins_[pins_.size() - 1]);
      pinMap_[pinPtr->name()] = pinPtr;
    }

    // Getters
    MacroClass macroClass() const { return macroClass_; }
    LefSite*         site() const { return macroSite_;  }
    std::string      name() const { return macroName_;  }

    float           sizeX() const { return sizeX_;      }
    float           sizeY() const { return sizeY_;      }

    float           origX() const { return origX_;      }
    float           origY() const { return origY_;      }

    const std::vector<LefPin>& pins() const { return pins_; }

    const LefPin* getPin(std::string& pinName) { return pinMap_[pinName]; }

    void printInfo() const;

  private:

    std::string    macroName_;
    MacroClass     macroClass_;
    LefSite* macroSite_;

    std::vector<LefPin> pins_;

    std::unordered_map<std::string, const LefPin*> pinMap_;

    float sizeX_;
    float sizeY_;
    
    float origX_;
    float origY_;
};

class dbNet
{
  public:

    dbNet() {}
    dbNet(int netID, std::string& netName) 
      : id_      (netID  ),
        netName_ (netName)
    {}

    // Getters
    int           id() const { return id_;      }
    std::string name() const { return netName_; }

    const std::vector<dbPin*>& pins() const { return pins_; }
  
    // Setters
    void setName(std::string& netName) { netName_ = netName; }
    void addPin(dbPin* pin) { pins_.push_back(pin); }

  private:

    int id_;

    std::string netName_;

    std::vector<dbPin*> pins_;
};

class dbPin
{
  public:

    // for external pins (when parsing PI/PO)
    dbPin(int pinID, 
          int netID,
          bool isPI,
          bool isPO,
          std::string&  pinName)
      : id_        (pinID    ),
        nid_       (netID    ),
        pinName_   (pinName  ),
        isPI_      (isPI     ),
        isPO_      (isPO     )
    {
      cid_        = INT_MAX;
      isExternal_ = true;
      lefPin_     = nullptr;
    }

    // for internal pins
    dbPin(int pinID, 
          int cellID, 
          int netID,
          std::string& pinName,
          const LefPin* lefPin)

      : id_        (pinID    ),
        cid_       (cellID   ),
        nid_       (netID    ),
        pinName_   (pinName  ),
        lefPin_    (lefPin   )
    {
      isExternal_ = false;
      isPI_       = false;
      isPO_       = false;
	
			cx_ = ( lefPin->lx() + lefPin->ly() ) / 2.0;
			cy_ = ( lefPin->ux() + lefPin->uy() ) / 2.0;
    }

    // Setters
    void setNet  (dbNet*   net)  { dbNet_  = net;          }
    void setCell (dbCell* cell)  { dbCell_ = cell;         }

    // Getters
    int               id() const { return id_;             }
    int              cid() const { return cid_;            }
    int              nid() const { return nid_;            }

    int            sizeX() const { return sizeX_;          }
    int            sizeY() const { return sizeY_;          }
    int               cx() const { return cx_;             }
    int               cy() const { return cy_;             }

    std::string  pinName() const { return pinName_;        }
    std::string portName() const { return lefPin_->name(); }

    dbCell*         cell() const { return dbCell_;         }
    dbNet*           net() const { return dbNet_;          }

    const LefPin* lefPin() const { return lefPin_;         }

    bool      isExternal() const { return isExternal_;     }
    bool            isPI() const { return isPI_;           }
    bool            isPO() const { return isPO_;           }

  private:

    int id_;
    int cid_;
    int nid_;

    // Size in dbu
    int sizeX_;
    int sizeY_;

		int cx_;
		int cy_;

    std::string pinName_;

    dbCell* dbCell_;
    dbNet*  dbNet_;

    bool isExternal_;
    bool isPI_;
    bool isPO_;

    const LefPin* lefPin_;
};

class dbCell
{
  public:

    dbCell() {}

    dbCell(int cellID, std::string& name, LefMacro* lefMacro) 
      : cellName_ (name), id_ (cellID), lefMacro_ (lefMacro)
    {}

    // Setters
    void setName       (std::string& name   ) { cellName_   = name;       }
    void setLefMacro   (LefMacro* lefMacro  ) { lefMacro_   = lefMacro;   }
    void setOrient     (Orient cellOrient   ) { cellOrient_ = cellOrient; }
    void setFixed      (bool isFixed        ) { isFixed_    = isFixed;    }
    void setLx         (int lx              ) { lx_         = lx;         }
    void setLy         (int ly              ) { ly_         = ly;         }
    void addPin        (dbPin* pin          ) { pins_.push_back(pin);     }

    // Getters
    std::string     name()  const { return cellName_;   }
    int               id()  const { return id_;         }
    int               lx()  const { return lx_;         }
    int               ly()  const { return ly_;         }
    LefMacro*   lefMacro()  const { return lefMacro_;   }
    bool         isFixed()  const { return isFixed_;    }
    Orient        orient()  const { return cellOrient_; }

    const std::vector<dbPin*>& pins() const { return pins_; }

  private:

    int id_;

    LefMacro* lefMacro_;

    std::string cellName_;

    bool isFixed_;

    Orient cellOrient_;

    std::vector<dbPin*> pins_;

    int lx_;
    int ly_;
};

class dbRow
{
  public:

    dbRow() {}
    dbRow(std::string& rowName,
          LefSite* lefSite,
          int lefUnit,
          int origX, 
          int origY, 
          int numSiteX, 
          int numSiteY,
          int stepX,
          int stepY) 
      : origX_    ( origX    ),
        origY_    ( origY    ),
        numSiteX_ ( numSiteX ),
        numSiteY_ ( numSiteY ),
        stepX_    ( stepX    ),
        stepY_    ( stepY    )
    {
      sizeX_ =
        static_cast<int>(lefSite->sizeX() * numSiteX * lefUnit);

      sizeY_ =
        static_cast<int>(lefSite->sizeY() * numSiteY * lefUnit);
    }

    // Getters
    std::string      name() const { return name_;    }
    LefSite*      lefSite() const { return lefSite_; }

    int    origX() const { return origX_;    }
    int    origY() const { return origY_;    }

    int numSiteX() const { return numSiteX_; }
    int numSiteY() const { return numSiteY_; }

    int    stepX() const { return stepX_;    }
    int    stepY() const { return stepY_;    }

    int    sizeX() const { return sizeX_;    }
    int    sizeY() const { return sizeY_;    }

  private:

    std::string name_;
    LefSite*    lefSite_;

    int origX_;
    int origY_;

    int numSiteX_;
    int numSiteY_;

    int stepX_;
    int stepY_;

    int sizeX_;
    int sizeY_;
};

class dbDie
{
  public:

    dbDie() {}

    // Setters
    void setCoordi(int lx, int ly, int ux, int uy)
    {
      lx_ = lx;
      ly_ = ly;
      ux_ = ux;
      uy_ = uy;
    }

    // Getters
    int lx() const { return lx_; }
    int ly() const { return ly_; }
    int ux() const { return ux_; }
    int uy() const { return uy_; }

  private:

    int lx_;
    int ly_;
    int ux_;
    int uy_;
};

class LefDefParser
{
  public:
    
    LefDefParser();

    // APIs
    void readLef     (const std::filesystem::path& path);                      // Read LEF
    void readDef     (const std::filesystem::path& path);                      // Read DEF
    void readVerilog (const std::filesystem::path& path);                      // Read Netlist (.v)

    // Getters
    std::vector<dbCell*> cells() const { return dbCellPtrs_; }                 // List of DEF COMPONENTS
    std::vector<dbPin*>   pins() const { return dbPinPtrs_;  }                 // List of Internal + External Pins
    std::vector<dbNet*>   nets() const { return dbNetPtrs_;  }                 // List of Nets
    std::vector<dbRow*>   rows() const { return dbRowPtrs_;  }                 // List of DEF ROWS

    const dbDie*           die() const { return &die_;       }                 // Ptr of dbDie

    std::string     designName() const { return designName_; }                 // Returns the top module name (from .v)

  private:

    // Tokenize strings of input file
    std::vector<std::string> tokenize(const std::filesystem::path& path,       // Input file (including file path)
                                            std::string_view dels,             // Delimiters
                                            std::string_view exps);            // Exceptions

    // LEF-related
    int dbUnit_;                                                               // LEF DATABASE MICRONS

    std::vector<LefMacro> macros_;                                             // List of LEF MACROS
    std::vector<LefSite>   sites_;                                             // List of LEF SITES

    std::unordered_map<std::string, LefMacro*> macroMap_;                      // Name - MACRO Table
    std::unordered_map<std::string, LefSite*>   siteMap_;                      // Name - SITE  Table

    void readLefPinShape (strIter& itr, const strIter& end, LefPin*     pin);  // Read One LEF Pin Shape 
    void readLefPin      (strIter& itr, const strIter& end, LefMacro* macro);  // Read One LEF Pin
    void readLefMacro    (strIter& itr, const strIter& end);                   // Read One LEF Macro

    void readLefSite     (strIter& itr, const strIter& end);                   // Read One LEF Site
    void readLefUnit     (strIter& itr, const strIter& end);                   // Read LEF DATABASE MICRONS

    void printLefStatistic() const;                                            // Print LEF Statistic (for debugging)

    std::unordered_map<std::string, MacroClass>   strToMacroClass_;            // String - enum MACRO_CLASS   Table
    std::unordered_map<std::string, SiteClass>    strToSiteClass_;             // String - enum SITE_CLASS    Table
    std::unordered_map<std::string, PinDirection> strToPinDirection_;          // String - enum PIN_DIRECTION Table
    std::unordered_map<std::string, PinUsage>     strToPinUsage_;              // String - enum PIN_USAGE     Table
    std::unordered_map<std::string, Orient>       strToOrient_;                // String - enum ORIENT        Table


    // Verilog-related
    std::string designName_;                                                   // Top Module name

    int numPI_;                                                                // Number of PI (Primary Input )
    int numPO_;                                                                // Number of PO (Primary Output)
    int numInst_;                                                              // Number of Instances
    int numNet_;                                                               // Number of Nets
    int numPin_;                                                               // Number of Pins
    int numDummy_;                                                             // Number of Dummy Cells
    // In the ICCAD 2015 supberblue benchmarks, 
    // there are some cells that exist in DEF
    // but not in the Verilog,
    // We will call them "Dummy Cells".

    std::vector<dbCell*> dbCellPtrs_;
    std::vector<dbCell>  dbCellInsts_;

    std::vector<dbPin*>  dbPinPtrs_;
    std::vector<dbPin>   dbPinInsts_;

    std::vector<dbNet*>  dbNetPtrs_;
    std::vector<dbNet>   dbNetInsts_;

    std::unordered_map<std::string, int> strToCellID_;
    std::unordered_map<std::string, int> strToNetID_;
    std::unordered_map<std::string, int> strToPinID_;

    // DEF-related
    int numRow_;
    int numDefComponents_;

    dbDie die_;

    std::vector<dbRow>   dbRowInsts_;
    std::vector<dbRow*>  dbRowPtrs_;

    void readDefDie          (strIter& itr, const strIter& end);
    void readDefRow          (strIter& itr, const strIter& end);
    void readDefOnePin       (strIter& itr, const strIter& end);
    void readDefPins         (strIter& itr, const strIter& end);
    void readDefOneComponent (strIter& itr, const strIter& end);
    void readDefComponents   (strIter& itr, const strIter& end);
};

};
