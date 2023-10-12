#ifndef R3B_THRESHOLD_SCAN
#define R3B_THRESHOLD_SCAN

#include "Common.h"
#include "AlpideDictionary.h"
#include <set>

class TDevice;
class TReadoutBoardMOSAIC;
class R3BAlpideDecoder;

/* ... I'm not doing sanity checks vs. nullptr ... 
 * Scan doesn't get involved in ownership of the TReadoutBoardMOSAIC object 
 * Important to have the TReadoutBoardMOSAIC instance properly constructed before! 
 * And board must be set in internal triggered mode. An instance of this class will send
 * software triggers and readout triggers using the board pointer.
 * Each device has a unique TReadoutBoardMOSAIC as it of April 2023.
 * ... If it changes later this needs to be refactored! --Klayze */

class R3BThresholdScan {
	friend class R3BAlpideDecoder;
public: 
	/* default scan params */
    static const int CHARGE_START    = 0;
    static const int CHARGE_STOP     = 100;
    static const int N_STEPS         = 50;
    static const int N_TRIGS_READOUT = 10;
    static const int N_TRIGS_SEND    = 10;
    static const int BUFFER_SIZE     = 1 << 24; /* 16 MB */
    static const int MAX_ROWS        = 512;

private:
    TDevice *device;
    TReadoutBoardMOSAIC* board;
	std::set<int> validChips;

    /* Name of the file which will store the information */
    std::string fileName;
    
    /* value of charge (VPULSEH - chargeStart) to be injected, at the start of the scan */
    int chargeStart;

    /* value of charge (VPULSEH - chargeStop) to be injected, at the end of the scan */
    int chargeStop;
    
    /* number of increments from initial injected charge to the final */
    int nSteps;

    /* Sending one software trigger to the device and nTrigs to the readout */
    int nTrigs;

public:
    R3BThresholdScan();
    R3BThresholdScan(TDevice* device);
    R3BThresholdScan(std::shared_ptr<TDevice> device);
	
	void FindValidChips();

	inline void SetDevice(std::shared_ptr<TDevice> device) {this->device = device.get();}
    inline void SetDevice(TDevice* device) {this->device = device;}
    void SetDevice();
    inline void SetBoard(TReadoutBoardMOSAIC* board) {this->board = board;}
    void SetBoard();
    void SetFileName(const char* fileName);
    void SetChargeParams(int chargeStart=CHARGE_START, int chargeStop=CHARGE_STOP, int nSteps=N_STEPS);
    void SetNTrigs(unsigned nTrigs);
    
     
    TDevice* GetDevice() const;
    TReadoutBoardMOSAIC* GetBoard() const;
    std::string GetFileName() const;
    std::tuple<int,int,int> GetChargeParams() const;
    int GetNTrigs() const;

    void FixParams(); 
	void DeactiveAllChips();

    void ActivateNextRow(const int chipId, const int row);

	void Init();
    bool Go();
	void Terminate();

    void Help();
};

#endif /* R3BThresholdScan */
