#include "TDevice.h"
#include "TAlpide.h"
#include "TReadoutBoardMOSAIC.h"
#include "TChipConfig.h"
#include "R3BThresholdScan.h"
#include "R3BStorePixHit.h"
#include "R3BAlpideDecoder.h"
#include "TFile.h"
#include "TTree.h"
#include <cassert>

using namespace std;

#if 0
#define WRITE_WR
#endif

R3BThresholdScan::R3BThresholdScan() :
    device(nullptr),
	board(nullptr),
    chargeStart(CHARGE_START),
    chargeStop(CHARGE_STOP),
    nSteps(N_STEPS), 
    nTrigs(N_TRIGS_READOUT),
    fileName("") {}

R3BThresholdScan::R3BThresholdScan(TDevice* device) :
    device(device),
    chargeStart(CHARGE_START),
    chargeStop(CHARGE_STOP),
    nSteps(N_STEPS),
    nTrigs(N_TRIGS_READOUT),
	fileName("") {
		int nBoards = device->GetNBoards(false);
		if(!nBoards) {
			cerr << "R3BThresholdScan::R3BThresholdScan(TDevice*) : no board found for TDevice* instance.\n";
			return;
		}
		if(nBoards > 1) {
			cerr << "R3BThresholdScan::R3BThresholdScan(TDevice*) : multiple boards found for TDevice* instance.\n";
			cerr << "Defaulting to the board at index 0.\n";
		}
		board = dynamic_cast<TReadoutBoardMOSAIC*>(device->GetBoard(0).get());
		FindValidChips();
	}

R3BThresholdScan::R3BThresholdScan(shared_ptr<TDevice> device) :
    device(device.get()),
    chargeStart(CHARGE_START),
    chargeStop(CHARGE_STOP),
    nSteps(N_STEPS),
    nTrigs(N_TRIGS_READOUT),
	fileName("") {
		int nBoards = device->GetNBoards(false);
		if(!nBoards) {
			cerr << "R3BThresholdScan::SetBoard() : no board found for TDevice* instance.\n";
			return;
		}
		if(nBoards > 1) {
			cerr << "R3BThresholdScan::R3BThresholdScan(TDevice*) : multiple boards found for TDevice* instance.\n";
			cerr << "Defaulting to the board at instance 0.\n";
		}
		board = dynamic_cast<TReadoutBoardMOSAIC*>(device->GetBoard(0).get());
		FindValidChips();
	}

void R3BThresholdScan::FindValidChips() {
	validChips.clear();
	for(int chipId=0; chipId<12; ++chipId) {
		try {
			if(device->IsValidChipId(chipId)) validChips.insert(chipId);
		} catch(exception& e) {}
	}
	printf("R3BThresholdScan::FindValidChips() : found %d working chips.", validChips.size());
}

void R3BThresholdScan::SetBoard() { // takes the board from a device handle
    if(!device) {
		cerr << "R3BThresholdScan::SetBoard() : no device handle set.\n";
		return;
	}
    int nBoards = device->GetNBoards(false);
    if(!nBoards) {
        cerr << "R3BThresholdScan::SetBoard() : no board found for TDevice* instance.\n";
        return;
    } 
	if(nBoards > 1) {
		cerr << __PRETTY_FUNCTION__ << " : multiple boards found for TDevice* instance.\n";
		cerr << "Defaulting to the board at instance 0.\n";
	}
    this->board = dynamic_cast<TReadoutBoardMOSAIC*>(device->GetBoard(0).get());
}

void R3BThresholdScan::SetFileName(const char *fileName) { this->fileName = string(fileName); }
void R3BThresholdScan::SetChargeParams(int chargeStart, int chargeStop, int nSteps) {
    this->chargeStart = chargeStart;
    this->chargeStop  = chargeStop;
    this->nSteps      = nSteps;
}

void R3BThresholdScan::SetNTrigs(unsigned nTrigs) {this->nTrigs = (int)nTrigs;}

TDevice* R3BThresholdScan::GetDevice() const { return device; }
TReadoutBoardMOSAIC* R3BThresholdScan::GetBoard() const { return board; }
string R3BThresholdScan::GetFileName() const { return fileName; }
tuple<int,int,int> R3BThresholdScan::GetChargeParams() const { return make_tuple(chargeStart, chargeStop, nSteps); }

void R3BThresholdScan::FixParams() {
    if(chargeStart >= chargeStop || nSteps==0) SetChargeParams(); 

    /* Charge is an int - if the chargeStep isn't an integer have to fix it */
    int chargeStep = (chargeStop - chargeStart)/nSteps;
    if(chargeStep == 0) {
        chargeStep = 1;
        nSteps = chargeStop - chargeStart - 1;
    }
}

void R3BThresholdScan::DeactiveAllChips() {
	for(int chipId : validChips) {
		device->GetChip(chipId)->WritePixRegAll(AlpidePixConfigReg::MASK_ENABLE, true);
		device->GetChip(chipId)->WritePixRegAll(AlpidePixConfigReg::PULSE_ENABLE, false);
	}
}

void R3BThresholdScan::ActivateNextRow(const int chipId, const int row) {
    assert((row < MAX_ROWS) && (row >= 0));
    assert(device->IsValidChipId(chipId));

    /* Mask the previous (i-1)-th row */
	if(row > 0) {
		device->GetChip(chipId)->WritePixRegRow(AlpidePixConfigReg::MASK_ENABLE, true, row-1);
		device->GetChip(chipId)->WritePixRegRow(AlpidePixConfigReg::PULSE_ENABLE, false, row-1);
	}
    /* Unmask the i-th row */
    device->GetChip(chipId)->WritePixRegRow(AlpidePixConfigReg::MASK_ENABLE, false, row);
    device->GetChip(chipId)->WritePixRegRow(AlpidePixConfigReg::PULSE_ENABLE, true, row);
}

void R3BThresholdScan::Init() {
	if(!board) {
		cerr << __PRETTY_FUNCTION__ << " , board not initialized!"; abort();
	}
	try {
		board->StartRun();
	}
	catch(exception& e) {
		cerr << e.what() << " , in the call to: " << __PRETTY_FUNCTION__ << endl;
	}
}

/* Main method, Init() has to be called before Go() */
bool R3BThresholdScan::Go() {
    int nBytes;
    FixParams();

	unsigned char* tempBuffer = (unsigned char*)malloc(BUFFER_SIZE);
	R3BAlpideDecoder decoder;

    int chargeStep = (chargeStop - chargeStart)/nSteps;
	int chargeInj;

    for(const int chipId : validChips) {
        /* Go over each chip in the device instance */
        uint16_t vpulseh = 0;
        device->GetChip(chipId)->ReadRegister(AlpideRegister::VPULSEH, vpulseh, true, true);
        if(vpulseh==0) vpulseh = TChipConfig::VPULSEH;

		DeactiveAllChips();
        
        for(int row = 0; row < MAX_ROWS; ++row) {
            /* Mask whole sensor except the area of interest which are rows: 
             * [stage*nRows, stage*(nRows-1)] */
            ActivateNextRow(chipId, row);
			
			R3BStorePixHit storeHits;
			
			/* FIXME rowFileName should be constructed from fileName field */
			string rowFileName = string("scan") + string("_chip") + to_string(chipId)+ string("_row") + to_string(row)+ string(".root"); 
			cout << "\n\nrowFileName = " << rowFileName << endl << endl; 
			storeHits.SetFileName(rowFileName);
			storeHits.Init();
			storeHits.fTree->Branch("CHARGE_INJ", &chargeInj);

			if(!storeHits.IsInitOk()) {
				cerr << "bool R3BThresholdScan::Go() - R3BStorePixHit uninitialized. ChipID = " << chipId << ", Row = " << row << endl;
				continue;
			}
            for(int step = 0; step <= nSteps; ++step) {   
				chargeInj = chargeStart + step * chargeStep;

                device->GetChip(chipId)->WriteRegister(AlpideRegister::VPULSEL, vpulseh - chargeInj);
                board->Trigger(1);
				
				for(int n=0; n<nTrigs; ++n) {
					// First call to TReadoutBoardMOSAIC::ReadEventData should poll trigger data,
					// which extracts timestamp
					// Subsequent calls should poll correct chip data (one call per chip) or empty-frame

					int readDataFlag = board->ReadEventData(nBytes, tempBuffer);
					if(readDataFlag == 0) {
						cerr << __PRETTY_FUNCTION__ << " : no data.\n";
						cerr << "ChipID : " << chipId << endl;
						abort();
					}
					else if(readDataFlag == MosaicDict::kTRGRECORDER_EVENT) {
						cerr << __PRETTY_FUNCTION__ << " : wrong data. Expected chip data, got trigger event. Waiting.\n";
						cerr << "ChipID    : " << chipId << endl;
						cerr << "Row       : " << row << endl;
						cerr << "Charge Inj: " << chargeInj << endl;
					}
					else if(readDataFlag == MosaicDict::kEMPTY_EVENT) {
						cerr << __PRETTY_FUNCTION__ << " : no data when polling for chip data. Got empty event. Waiting.\n";
						cerr << "ChipID    : " << chipId << endl;
						cerr << "Row       : " << row << endl;
						cerr << "Charge Inj: " << chargeInj << endl;
					}
					else {
						decoder.DecodeEvent(tempBuffer, nBytes);
						storeHits.Fill(decoder);
					}

#ifdef WRITE_WR
					// Write WR timestamp first
					struct timespec tnow;

					//byteswap data 
					tempBuffer[nBytes-1] = 0xff;
					while((nBytes & 0x3) != 0) {
						tempBuffer[nBytes++] = 0xff;
					}

					{
						uint32_t* p = (uint32_t *)&tempBuffer[64];
						for(int i = 64; i < nBytes; i+=4) {
							uint32_t val =
								((*p & 0x000000ff) << 24)
								| ((*p & 0x0000ff00) <<  8)
								| ((*p & 0x00ff0000) >>  8)
								| ((*p & 0xff000000) >> 24);
							*p = val;
							p++;
						}
					}
#endif
				}
            } // end of step loop

			storeHits.Terminate(); // writes and saves the rootfile
        } // end of row loop

    } // end of chip loop

	free(tempBuffer);
	return true;
}

void R3BThresholdScan::Terminate() {
	if(!board) return;
	try {
		board->StopRun(); 	
	}
	catch(exception& e) {
		cerr << e.what() << " in function: " << __PRETTY_FUNCTION__ << endl;
	}
}

void R3BThresholdScan::Help() {
    /* FIXME */
}
