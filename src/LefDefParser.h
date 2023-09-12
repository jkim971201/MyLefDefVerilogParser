#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace LefDefDB
{

// For Parsing LEF
typedef std::vector<std::string>::iterator strIter;

enum MacroClass    {CORE, CORE_SPACER, PAD, BLOCK, ENDCAP};
enum SiteClass     {CORE_SITE};
enum PinDirection  {INPUT, OUTPUT, INOUT};
enum PinUsage      {SIGNAL, POWER};
enum CellOrient    {N, S, FN, FS};  // W E FW FE is not supported

class LefMacro;
class LefPin;
class LefSite;

struct LefRect
{
  float lx;
  float ly;
  float ux;
  float uy;

  LefRect(float lx, float ly, float ux, float uy)
    : lx(lx), ly(ly), ux(ux), uy(uy) {}
};

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
    LefMacro* macro() const { return lefMacro_; }

    std::string name() const { return pinName_; }

    PinUsage     usage()     const { return pinUsage_; }
    PinDirection direction() const { return pinDir_;   }

    const std::vector<LefRect>& lefRect() const { return lefRect_; }

    std::pair<float, float> getPinCoordi() const;

  private:

    LefMacro* lefMacro_;

    std::string   pinName_;
    PinUsage      pinUsage_;
    PinDirection  pinDir_;

    std::vector<LefRect> lefRect_;
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

    LefSite* site() const { return macroSite_; }

    std::string name() const { return macroName_; }

    float sizeX() const { return sizeX_; }
    float sizeY() const { return sizeY_; }

    float origX() const { return origX_; }
    float origY() const { return origY_; }

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

    dbPin() {}

    // for internal pins
    dbPin(int pinID, 
          int cellID, 
          int netID,
          std::string pinName,
          const LefPin* lefPin)

      : id_        (pinID    ),
        cid_       (cellID   ),
        nid_       (netID    ),
        pinName_   (pinName  ),
        lefPin_    (lefPin   )
    {}

    // Setters
    void setNet(dbNet* net)      { dbNet_  = net;          }
    void setCell(dbCell* cell)   { dbCell_ = cell;         }

    // Getters
    int         id()       const { return id_;             }
    int         cid()      const { return cid_;            }
    int         nid()      const { return nid_;            }

    std::string pinName()  const { return pinName_;        }
    std::string portName() const { return lefPin_->name(); }

    dbCell* cell()   const { return dbCell_;  }
    dbNet*  net()    const { return dbNet_;   }

    const LefPin* lefPin() const { return lefPin_;  }

  private:

    int id_;
		int cid_;
		int nid_;

    std::string pinName_;

    dbCell* dbCell_;
    dbNet*  dbNet_;

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
    void setName(std::string& name) { cellName_ = name; }

    void setLefMacro(LefMacro* lefMacro) { lefMacro_ = lefMacro; }

    void setCellOrient(CellOrient cellOrient) { cellOrient_ = cellOrient; }

    void setFixed(bool isFixed) { isFixed_ = isFixed; }

    void addPin(dbPin* pin) { pins_.push_back(pin); }

    // Getters
    std::string name()     const { return cellName_;   }
    int         id()       const { return id_;         }

    LefMacro*   lefMacro() const { return lefMacro_;   }

    bool       isFixed()   const { return isFixed_;    }
    CellOrient orient()    const { return cellOrient_; }

    const std::vector<dbPin*>& pins() const { return pins_; }

		int         lx()       const { return lx_;         }
		int         ly()       const { return ly_;         }

  private:

    int id_;

    LefMacro* lefMacro_;

    std::string cellName_;

    bool isFixed_;

    CellOrient cellOrient_;

    std::vector<dbPin*> pins_;

		int lx_;
		int ly_;
};

class dbRow
{
	public:

	private:
}

class LefDefParser
{
  public:
    
    LefDefParser();

    void readLef(const std::filesystem::path& path);

    void readVerilog(const std::filesystem::path& path);

    void readDef(const std::filesystem::path& path);

		std::vector<dbCell*> cells() const { return dbCellPtrs_; }
		std::vector<dbPin*>  pins()  const { return dbPinPtrs_;  }
		std::vector<dbNet*>  nets()  const { return dbNetPtrs_;  }
		std::vector<dbRow*>  rows()  const { return dbRowPtrs_;  }

  private:

    std::vector<std::string> tokenize(const std::filesystem::path& path, 
                                            std::string_view dels,
                                            std::string_view exps);

    // LEF-related
    std::unordered_map<std::string, LefMacro*> macroMap_;
    std::unordered_map<std::string, LefSite*>  siteMap_;

    void readLefPort (strIter& itr, const strIter& end, LefPin* pin);
    void readLefPin  (strIter& itr, const strIter& end, LefMacro* macro);
    void readLefMacro(strIter& itr, const strIter& end);

    void readLefSite (strIter& itr, const strIter& end);
    void readLefUnit (strIter& itr, const strIter& end);

    void printLefStatistic() const;

    std::vector<LefMacro> macros_;
    std::vector<LefSite>  sites_;

    std::unordered_map<std::string, MacroClass>   strToMacroClass_;
    std::unordered_map<std::string, SiteClass>    strToSiteClass_;
    std::unordered_map<std::string, PinDirection> strToPinDirection_;
    std::unordered_map<std::string, PinUsage>     strToPinUsage_;

    int dbUnit_;

    // Verilog-related
    int numPI_;
    int numPO_;
    int numInst_;
    int numNet_;
    int numPin_;

    std::vector<dbCell*> dbCellPtrs_;
    std::vector<dbCell>  dbCellInsts_;

    std::vector<dbPin*>  dbPinPtrs_;
    std::vector<dbPin>   dbPinInsts_;

    std::vector<dbNet*>  dbNetPtrs_;
    std::vector<dbNet>   dbNetInsts_;

    std::unordered_map<std::string, int> strToCellID_;
    std::unordered_map<std::string, int> strToNetID_;

		// DEF-related
		std::vector<dbRow>   dbRowInsts_;
		std::vector<dbRow*>  dbRowPtrs_;
};

};
