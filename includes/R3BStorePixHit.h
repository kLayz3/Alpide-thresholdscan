#ifndef R3B_STOREPIXHIT_H
#define R3B_STOREPIXHIT_H

/* >> Martin Bajzek
 * For each hit pixel, the following information is stored:
 * chip Id, row, column, bunch crossing counter (from chip), trigger 
 * counter (from the readout board). 
 * Only valid hits (i.e. with no bad flag) are stored. See R3BPixHit.h for 
 * the list of bad flags. */

#include <memory>
#include <stdint.h>
#include <string>
#include "Common.h"

class R3BPixHit;
class R3BAlpideDecoder;
class TTree;
class TFile;

class R3BStorePixHit {
	friend class R3BAlpideDecoder;
	friend class R3BThresholdScan;
public:
    typedef struct {
		uint32_t boardIndex;
        uint32_t chipId;
        uint32_t row;
        uint32_t col;
        uint32_t bunchNum;
		uint32_t thi;
        uint32_t tlo;
    } TDataSummary;

private:
    TTree* fTree; // the ROOT TTree container
    TFile* fFile; // Output file

    bool fSuccessfulInit;     // boolean to monitor the success of the initialization
    long fNEntriesAutoSave;   // max entries in the buffer after which TTree::AutoSave() is automatically used
	TDataSummary fData;       // struct instance whose fields will be branch addresses for TTree
	std::string fOutFileName; // output file name that will store the TTree
	std::string fTreeTitle;   // title of the TTree

public:
    R3BStorePixHit();
    ~R3BStorePixHit();

    // Set the name of the output ROOT TTree file, the TTree title
    inline void SetFileName(std::string fileName) {fOutFileName = fileName;} 
    inline void SetFileName(const char* fileName) {fOutFileName = std::string(fileName);} 

    // Set the number of entries to be used by TTree::AutoSave(), default 10000
    void SetCyclicAutoSave(long nEntries = 10000);
	
    void Init();
    inline bool IsInitOk() const {return fSuccessfulInit;}

    void Fill(const R3BPixHit& hit);			// fill the ROOT TTree with the information from a hit object
	void Fill(const R3BAlpideDecoder& decoder); // fill the ROOT TTree with the information from the decoder instance

    void Terminate(); 

private:
    void SetDataSummary(const R3BPixHit& hit);
};

#endif
