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

class dbNet
{
	public:

	
	private:
};

class dbPin
{
	public:

		dbPin() {}
		dbPin(dbCell* dbCell, dbNet* dbNet, std::string pinName)
			: dbCell_    (dbCell  ), 
			  dbNet_     (dbNet   ),
			  pinName_   (pinName ) 
		{}

		// Setters
		void setLefPin(LefPin* lefPin) { lefPin_ = lefPin; }

		// Getters
		std::string   name()   const { return pinName_; }

		dbCell* cell() const { return dbCell_; }
		dbNet*  net()  const { return dbNet_;  }

		const LefPin* lefPin() const { return lefPin_;  }

	private:

		std::string pinName_;

		dbCell* dbCell_;
		dbNet*  dbNet_;

		LefPin* lefPin_;
};

class dbCell
{
	public:

		dbCell() {}
		dbCell(const LefMacro* lefMacro, std::string cellName) 
			: lefMacro_  (lefMacro), 
			  cellName_  (cellName) 
		{}

		// Setters
		void setSizeX(int sizeX) { sizeX_ = sizeX; }
		void setSizeY(int sizeY) { sizeY_ = sizeY; }

		void setCellOrient(CellOrient cellOrient) { cellOrient_ = cellOrient; }

		void setFixed(bool isFixed) { isFixed_ = isFixed; }

		void addPin(dbPin* pin) { pins_.push_back(pin); }

		// Getters
		const LefMacro* lefMacro() const { return lefMacro_; }

		int sizeX() const { return sizeX_; }
		int sizeY() const { return sizeY_; }

		bool isFixed() const { return isFixed_; }

		CellOrient orient() const { return cellOrient_; }

		const std::vector<dbPin*>& pins() const { return pins_; }

	private:

		const LefMacro* lefMacro_;

		std::string cellName_;

		int sizeX_;
		int sizeY_;

		bool isFixed_;

		CellOrient cellOrient_;

		std::vector<dbPin*> pins_;
};

class LefPin
{
	public: 

		LefPin(std::string pinName, LefMacro* lefMacro);

		// Setters
		void setPinUsage    (PinUsage     pinUsage) { pinUsage_ = pinUsage; }
		void setPinDirection(PinDirection pinDir)   { pinDir_   = pinDir;   }

		void addLefRect(LefRect rect) { lefRect_.push_back(rect); }

		// Getters
		const LefMacro* macro() const { return lefMacro_; }

		std::string name() const { return pinName_; }

		PinUsage     usage()     const { return pinUsage_; }
		PinDirection direction() const { return pinDir_;   }

		const std::vector<LefRect>& lefRect() const { return lefRect_; }

	private:

		const LefMacro* lefMacro_;

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
						float sizeY);

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
		
		LefMacro(std::string macroName);

		// Setters
		void setClass(MacroClass macroClass)  { macroClass_ = macroClass; }
		void setSite (const LefSite* lefSite) { macroSite_  = lefSite;    }
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

		const LefSite* site() const { return macroSite_; }

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
		const LefSite* macroSite_;

		std::vector<LefPin> pins_;

		std::unordered_map<std::string, const LefPin*> pinMap_;

		float sizeX_;
		float sizeY_;
		
		float origX_;
		float origY_;
};

class LefDefParser
{
	public:
		
		LefDefParser();

		void readLef(const std::filesystem::path& path);

		void readVerilog(const std::filesystem::path& path);

		void readDef(const std::filesystem::path& path);

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
		std::vector<dbCell*> dbCellPtr_;
		std::vector<dbCell>  dbCellInst_;

		std::vector<dbPin*>  dbPinPtr_;
		std::vector<dbPin>   dbPinInst_;

		std::vector<dbNet*>  dbNetPtr_;
		std::vector<dbNet>   dbNetInst_;
};

};
