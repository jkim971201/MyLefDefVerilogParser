#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <climits>

namespace LefDefDB
{

// For Parsing
typedef std::vector<std::string>::iterator strIter;

enum MacroClass   {CORE, CORE_SPACER, PAD, BLOCK, ENDCAP};
enum SiteClass    {CORE_SITE};
enum PinDirection {INPUT, OUTPUT, INOUT};
enum PinUsage     {SIGNAL, POWER, GROUND, CLOCK};
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
class dbIO;

class LefPin
{
  public: 

    LefPin(std::string pinName, LefMacro* lefMacro)
      : pinName_  (pinName   ),
        lefMacro_ (lefMacro  )
    {}

    // Setters
    void setPinUsage    (PinUsage     pinUsage) { pinUsage_ = pinUsage;     }
    void setPinDirection(PinDirection pinDir  ) { pinDir_   = pinDir;       }

    void addLefRect     (LefRect      rect    ) { lefRect_.push_back(rect); }

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

    std::string  macroName_;
    MacroClass   macroClass_;
    LefSite*     macroSite_;

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
    void addPin (dbPin* pin) { pins_.push_back(pin); }

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
          int  ioID,
          std::string&  pinName)
      : id_        (pinID   ),
        nid_       (netID   ),
        ioid_      (ioID    ),
        pinName_   (pinName )
    {
      cid_        = INT_MAX;
      lefPin_     = nullptr;
      dbCell_     = nullptr;

      isExternal_ = true;
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
      ioid_ = INT_MAX;
      dbIO_ = nullptr;
  
      offsetX_ = ( lefPin->lx() + lefPin->ux() ) / 2;
      offsetY_ = ( lefPin->ly() + lefPin->uy() ) / 2;

      isExternal_ = false;
    }

    // Setters
    void setNet     (dbNet*   net) { dbNet_  = net;      }
    void setCell    (dbCell* cell) { dbCell_ = cell;     }
    void setIO      (dbIO*     io) { dbIO_   = io;       }

    void setCx      (int       cx) { cx_     = cx;       }
    void setCy      (int       cy) { cy_     = cy;       }

    void setOffsetX (int  offsetX) { offsetX_ = offsetX; }
    void setOffsetY (int  offsetY) { offsetY_ = offsetY; }

    // Getters
    int               id() const { return id_;         }
    int              cid() const { return cid_;        }
    int              nid() const { return nid_;        }
    int             ioid() const { return ioid_;       }

    int               cx() const { return cx_;         }
    int               cy() const { return cy_;         }
    int          offsetX() const { return offsetX_;    }
    int          offsetY() const { return offsetY_;    }

    std::string     name() const { return pinName_;    }

    dbCell*         cell() const { return dbCell_;     }
    dbNet*           net() const { return dbNet_;      }
    dbIO*             io() const { return dbIO_;       }

    bool      isExternal() const { return isExternal_; }
    const LefPin* lefPin() const { return lefPin_;     }

  private:

    int   id_; // Pin  ID
    int  cid_; // Cell ID
    int  nid_; // Net  ID
    int ioid_; // IO   ID

    // dbu
    int cx_;
    int cy_;

    int offsetX_;
    int offsetY_;

    std::string pinName_;

    dbCell* dbCell_;
    dbNet*  dbNet_;
    dbIO*   dbIO_;

    bool isExternal_;
    const LefPin* lefPin_;
};


class dbIO
{
  public:

    // If Verilog is read first (before reading .def)
    // Use this constructor
    dbIO(int ioID, 
         PinDirection direction,
         std::string& name) 
      : id_         (      ioID),
        direction_  ( direction),
        ioName_     (      name) 
    {}

    // If DEF is read first (before reading .v)
    // Use this constructor
    dbIO(int ioID, 
         int origX,
         int origY,
         int offsetLx,  
         int offsetLy,  
         int offsetUx,  
         int offsetUy,  
         bool isFixed,
         Orient orient,
         PinDirection direction,
         std::string& name) 
      : id_         (      ioID),
        origX_      (     origX),
        origY_      (     origY),
        offsetLx_   (  offsetLx),
        offsetLy_   (  offsetLy),
        offsetUx_   (  offsetUx),
        offsetUy_   (  offsetUy),
        orient_     (    orient),
        direction_  ( direction),
        ioName_     (      name) 
    {}

    // Getters
    int           id() const { return id_;       }
    int        origX() const { return origX_;    }
    int        origY() const { return origY_;    }
    int     offsetLx() const { return offsetLx_; }
    int     offsetLy() const { return offsetLy_; }
    int     offsetUx() const { return offsetUx_; }
    int     offsetUy() const { return offsetUy_; }
    bool     isFixed() const { return isFixed_;  }

    dbPin*       pin() const { return pin_;      }
    std::string name() const { return ioName_;   }

    Orient          orient() const { return orient_;    }
    PinDirection direction() const { return direction_; }

    // Setters
    void setOrigX     (int     origX) { origX_    = origX;    }
    void setOrigY     (int     origY) { origY_    = origY;    }
    void setOffsetLx  (int  offsetLx) { offsetLx_ = offsetLx; }
    void setOffsetLy  (int  offsetLy) { offsetLy_ = offsetLy; }
    void setOffsetUx  (int  offsetUx) { offsetUx_ = offsetUx; }
    void setOffsetUy  (int  offsetUy) { offsetUy_ = offsetUy; }

    void setLocation(int origX, int origY, 
                     int offsetLx, int offsetLy, 
                     int offsetUx, int offsetUy)
    {
      origX_    = origX;
      origY_    = origY;
      offsetLx_ = offsetLx;
      offsetLy_ = offsetLy;
      offsetUx_ = offsetUx;
      offsetUy_ = offsetUy;

      pin_->setCx(origX);
      pin_->setCy(origY);

      pin_->setOffsetX( (offsetLx + offsetUx) / 2 );
      pin_->setOffsetY( (offsetLy + offsetUy) / 2 );
    }

    void setOrient    (Orient orient) { orient_   = orient;   }
    void setFixed     (bool  isFixed) { isFixed_  = isFixed;  }
    // These are written in DEF PINS
    
    void setPin       (dbPin* pin   ) { pin_      = pin;      }

  private:

    int id_;

    int origX_;
    int origY_;

    int offsetLx_;
    int offsetLy_;
    int offsetUx_;
    int offsetUy_;

    bool isFixed_;

    Orient       orient_;
    PinDirection direction_;
    std::string  ioName_;

    dbPin* pin_;
};


class dbCell
{
  public:

    dbCell() {}

    dbCell(int cellID, std::string& name, LefMacro* lefMacro) 
      : cellName_ (name), id_ (cellID), lefMacro_ (lefMacro)
    {
      if(lefMacro_->macroClass() == MacroClass::CORE)
      {
        isStdCell_ = true;
        isMacro_   = false;
      }
      if(lefMacro_->macroClass() == MacroClass::BLOCK)
      {
        isMacro_   = true;
        isStdCell_ = false;
      }

      isDummy_ = false;
    }

    // Setters
    void setName       (std::string&  name  ) { cellName_   = name;       }
    void setLefMacro   (LefMacro* lefMacro  ) { lefMacro_   = lefMacro;   }
    void setOrient     (Orient  cellOrient  ) { cellOrient_ = cellOrient; }
    void setFixed      (bool       isFixed  ) { isFixed_    = isFixed;    }
    void setDummy      (bool       isDummy  ) { isDummy_    = isDummy;    }
    void setLx         (int             lx  ) { lx_         = lx;         }
    void setLy         (int             ly  ) { ly_         = ly;         }
    void setDx         (int             dx  ) { dx_         = dx;         }
    void setDy         (int             dy  ) { dy_         = dy;         }
    void addPin        (dbPin*         pin  ) { pins_.push_back(pin);     }

    // Getters
    std::string     name()  const { return cellName_;   }
    int               id()  const { return id_;         }
    int               lx()  const { return lx_;         }
    int               ly()  const { return ly_;         }
    int               ux()  const { return lx_ + dx_;   }
    int               uy()  const { return ly_ + dy_;   }
    int               dx()  const { return dx_;         }
    int               dy()  const { return dy_;         }
    int64_t         area()  const { return static_cast<int64_t>(dx_) 
                                         * static_cast<int64_t>(dy_); }

    LefMacro*   lefMacro()  const { return lefMacro_;   }
    bool         isFixed()  const { return isFixed_;    }
    bool       isStdCell()  const { return isStdCell_;  }
    bool         isMacro()  const { return isMacro_;    }
    bool         isDummy()  const { return isDummy_;    }
    Orient        orient()  const { return cellOrient_; }

    const std::vector<dbPin*>& pins() const { return pins_; }

  private:

    int id_;

    LefMacro* lefMacro_;

    std::string cellName_;

    bool isFixed_;

    bool isMacro_;
    bool isDummy_;
    bool isStdCell_;

    Orient cellOrient_;

    std::vector<dbPin*> pins_;

    int lx_;
    int ly_;

    int dx_;
    int dy_;
};

class dbRow
{
  public:

    dbRow() {}
    dbRow(std::string& rowName,
          LefSite* lefSite,
          int dbUnit,
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
        static_cast<int>( lefSite->sizeX() * numSiteX * dbUnit );

      sizeY_ =
        static_cast<int>( lefSite->sizeY() * numSiteY * dbUnit );
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

    void setCoreCoordi(int lx, int ly, int ux, int uy)
    {
      coreLx_ = lx;
      coreLy_ = ly;
      coreUx_ = ux;
      coreUy_ = uy;
    }

    // Getters
    int lx()     const { return lx_;     }
    int ly()     const { return ly_;     }
    int ux()     const { return ux_;     }
    int uy()     const { return uy_;     }

    int coreLx() const { return coreLx_; }
    int coreLy() const { return coreLy_; }
    int coreUx() const { return coreUx_; }
    int coreUy() const { return coreUy_; }

    int64_t area() const { return static_cast<int64_t>(ux_ - lx_)
                                * static_cast<int64_t>(uy_ - ly_); }
    
    int64_t coreArea() const { return static_cast<int64_t>(coreUx_ - coreLx_)
                                    * static_cast<int64_t>(coreUy_ - coreLy_); }

  private:

    int lx_;
    int ly_;
    int ux_;
    int uy_;

    int coreLx_;
    int coreLy_;
    int coreUx_;
    int coreUy_;
};

class LefDefParser
{
  public:
    
    LefDefParser();

    // APIs
    void readLef     (const std::filesystem::path& path);                      // Read LEF
    void readDef     (const std::filesystem::path& path);                      // Read DEF
    void readVerilog (const std::filesystem::path& path);                      // Read Netlist (.v)
    void printInfo   ();                                                       // Print Technology & Design Information

    // Getters
    std::vector<dbCell*> cells() const { return dbCellPtrs_; }                 // List of DEF COMPONENTS
    std::vector<dbIO*>     ios() const { return dbIOPtrs_;   }                 // List of DEF PINS 
    std::vector<dbPin*>   pins() const { return dbPinPtrs_;  }                 // List of Internal + External Pins
    std::vector<dbNet*>   nets() const { return dbNetPtrs_;  }                 // List of Nets
    std::vector<dbRow*>   rows() const { return dbRowPtrs_;  }                 // List of DEF ROWS
    const dbDie*           die() const { return &die_;       }                 // Ptr of dbDie
		int                 dbUnit() const { return dbUnit_;     }                 // Get DB Unit (normally 1000 / 2000)

    std::string     designName() const { return designName_; }                 // Returns the top module name (from .v)

  private:

    // Tokenize strings of input file
    std::vector<std::string> tokenize(const std::filesystem::path& path,       // Input file (including file path)
                                            std::string_view dels,             // Delimiters
                                            std::string_view exps);            // Exceptions

    bool ifReadLef_;                                                           // LEF     Flag
    bool ifReadVerilog_;                                                       // Verilog Flag
    bool ifReadDef_;                                                           // DEF     Flag

		void reset();                                                              // Reset Function (clear or initialize all db)

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

		std::set<std::string>                         lefList_;                    // Set of LEF File name that already read

    // Verilog-related
    std::string designName_;                                                   // Top Module name

    int numPI_;                                                                // Number of PI (Primary Input )
    int numPO_;                                                                // Number of PO (Primary Output)
    int numIO_;                                                                // Number of IO (PI + PO)
    int numInst_;                                                              // Number of Total Instances
    int numStdCell_;                                                           // Number of Standard Cells
    int numMacro_;                                                             // Number of Macros 
    int numNet_;                                                               // Number of Nets
    int numPin_;                                                               // Number of Pins
    int numDummy_;                                                             // Number of Dummy Cells
    // In the ICCAD 2015 superblue benchmarks, 
    // there are some cells that exist in DEF
    // but not in the Verilog,
    // We will call them "Dummy Cells".

    std::vector<dbCell*> dbCellPtrs_;                                          // List of dbCell Pointer
    std::vector<dbCell>  dbCellInsts_;                                         // List of dbCell Instance

    std::vector<dbPin*>  dbPinPtrs_;                                           // List of dbPin Pointer
    std::vector<dbPin>   dbPinInsts_;                                          // List of dbPin Instance

    std::vector<dbIO*>   dbIOPtrs_;                                            // List of dbIO Pointer
    std::vector<dbIO>    dbIOInsts_;                                           // List of dbIO Instance

    std::vector<dbNet*>  dbNetPtrs_;                                           // List of dbNet Pointer
    std::vector<dbNet>   dbNetInsts_;                                          // List of dbNet Instance

    std::unordered_map<std::string, int> strToCellID_;                         // CellName - CellID Table
    std::unordered_map<std::string, int> strToNetID_;                          //  NetName -  NetID Table
    std::unordered_map<std::string, int> strToPinID_;                          //  PinName -  PinID Table
    std::unordered_map<std::string, int> strToIOID_;                           //   IOName -   IOID Table

    // DEF-related
    int numRow_;                                                               // Number of ROWS       in DEF
    int numDefComps_;                                                          // Number of COMPONENTS in DEF

    int64_t sumTotalInstArea_;                                                 // Sum of Total instance area (StdCell + Macro)
    int64_t sumStdCellArea_;                                                   // Sum of cell area
    int64_t sumMacroArea_;                                                     // Sum of macro block area

    float util_;                                                               // TotalInstArea / DieArea
    float density_;                                                            // MovableArea / (DieArea - FixedArea)
																																							 // e.g. MovableArea = StdCellArea
																																							 //      FixedArea   = MacroArea

    dbDie die_;                                                                // Instance of dbDie

    std::vector<dbRow*>  dbRowPtrs_;                                           // List of Row Pointers
    std::vector<dbRow>   dbRowInsts_;                                          // List of Row Instances

    void readDefDie          (strIter& itr, const strIter& end);               // Read One DEF DIE
    void readDefRow          (strIter& itr, const strIter& end);               // Read One DEF ROW
    void readDefOnePin       (strIter& itr, const strIter& end);               // Read One DEF PIN
    void readDefPins         (strIter& itr, const strIter& end);               // Read DEF PINS
    void readDefOneComponent (strIter& itr, const strIter& end);               // Read One DEF COMPONENT
    void readDefComponents   (strIter& itr, const strIter& end);               // Read DEF COMPONENTS
};

};
