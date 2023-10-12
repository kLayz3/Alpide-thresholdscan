#include "R3BPixHit.h"
#include <iostream>
#include <climits>

using namespace std;

R3BPixHit::R3BPixHit() :
    fBoardIndex(0),
    fChipId(ILLEGAL_CHIP_ID),
    fRegion(32),
    fDcol(0),
    fAddress(0),
    fFlag(AlpidePixFlag::kUNKNOWN),
    fBunchCounter(0),
	thi(0),
	tlo(0) {}
	
R3BPixHit::R3BPixHit(const R3BPixHit& rhs) :
	fBoardIndex(rhs.fBoardIndex),
	fChipId(rhs.fChipId),
	fRegion(rhs.fRegion),
	fBunchCounter(rhs.fBunchCounter),
	thi(rhs.thi),
	tlo(rhs.tlo),
	// defaulted fields :
	fDcol(0),
	fAddress(0),
	fFlag(AlpidePixFlag::kUNKNOWN) {}

R3BPixHit& R3BPixHit::operator=(const R3BPixHit& rhs) {
	fBoardIndex   = rhs.fBoardIndex;
	fChipId       = rhs.fChipId;
	fRegion       = rhs.fRegion;
	fDcol         = rhs.fDcol;
	fAddress      = rhs.fAddress;
	fFlag         = rhs.fFlag;
	fBunchCounter = rhs.fBunchCounter;
	tlo           = rhs.tlo;
	thi           = rhs.thi;
	return *this;
}

void R3BPixHit::SetChipId(const uint32_t value) {
    if(value == ILLEGAL_CHIP_ID) {
        cerr << "R3BPixHit::SetChipId() - Warning, illegal chip id = 15" << endl;
        fFlag = AlpidePixFlag::kBAD_CHIPID;
   }
   fChipId = value;
}

void R3BPixHit::SetRegion(const uint32_t value) {
    if(value > common::MAX_REGION) {
        cerr << "R3BPixHit::SetRegion() - Warning, region > 31" << endl;
        fFlag = AlpidePixFlag::kBAD_REGIONID;
    }
    fRegion = value;
}

// encoder_id = [0,15] as each region has 16 double-columns which
void R3BPixHit::SetDoubleColumn(const uint32_t encoder_id) {
    uint32_t dcol = encoder_id + GetRegion() * common::NDCOL_PER_REGION;
    if(dcol > common::MAX_DCOL) {
        cerr << "R3BPixHit::SetDoubleColumn() - Warning, double column > 511" << endl;
        fFlag = AlpidePixFlag::kBAD_DCOLID;
	}
    fDcol = dcol;
}

void R3BPixHit::SetAddress(const uint32_t value) {
    if(value > common::MAX_ADDR) {
        cerr << "R3BPixHit::SetAddress() - Warning, address > 1023" << endl;
        fFlag = AlpidePixFlag::kBAD_ADDRESS;
	}
    fAddress = value;
}

uint32_t R3BPixHit::GetChipId() const {
	if(fChipId == ILLEGAL_CHIP_ID) 
		cerr << "R3BPixHit::GetChipId() - Warning, illegal chip id = 15" << endl;
	if(fFlag == AlpidePixFlag::kBAD_CHIPID) 
		cerr << "R3BPixHit::GetChipId() - Warning, AlpidePixFlag::kBAD_CHIPID" << endl;
	return fChipId;
}

unsigned R3BPixHit::GetRegion() const {
	if (fRegion > common::MAX_REGION) 
		cerr << "R3BPixHit::GetRegion() - Warning, region > 31" << endl;
	if (fFlag == AlpidePixFlag::kBAD_REGIONID) 
		cerr << "R3BPixHit::GetRegion() - Warning, AlpidePixFlag::kBAD_REGIONID" << endl;
	return fRegion;
}

unsigned R3BPixHit::GetDoubleColumn() const {
	if(fDcol > common::MAX_DCOL) 
		cerr << "R3BPixHit::GetDoubleColumn() - Warning, double column > 511" << endl;
	if(fFlag == AlpidePixFlag::kBAD_DCOLID) 
		cerr << "R3BPixHit::GetDoubleColumn() - Warning, AlpidePixFlag::kBAD_DCOLID" << endl;
	return fDcol;
}

unsigned R3BPixHit::GetAddress() const {
	if(fAddress > common::MAX_ADDR) 
		cerr << "R3BPixHit::GetAddress() - Warning, address > 1023" << endl;
	if(fFlag == AlpidePixFlag::kBAD_ADDRESS) 
		cerr << "R3BPixHit::GetAddress() - Warning, AlpidePixFlag::kBAD_ADDRESS" << endl;
    return fAddress;
}

AlpidePixFlag R3BPixHit::GetPixFlag() const {
        if (fFlag == AlpidePixFlag::kBAD_ADDRESS) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kBAD_ADDRESS" << endl;
        if (fFlag == AlpidePixFlag::kBAD_DCOLID) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kBAD_DCOLID" << endl;
        if (fFlag == AlpidePixFlag::kBAD_REGIONID) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kBAD_REGIONID" << endl;
        if (fFlag == AlpidePixFlag::kBAD_CHIPID) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kBAD_CHIPID" << endl;
        if (fFlag == AlpidePixFlag::kSTUCK) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kSTUCK" << endl;
        if (fFlag == AlpidePixFlag::kDEAD) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kDEAD" << endl;
        if (fFlag == AlpidePixFlag::kINEFFICIENT) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kINEFFICIENT" << endl;
        if (fFlag == AlpidePixFlag::kHOT) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kHOT" << endl;
        if (fFlag == AlpidePixFlag::kUNKNOWN) 
            cerr << "R3BPixHit::GetPixFlag() - Warning, AlpidePixFlag::kUNKNOWN" << endl;
    return fFlag;
}

bool R3BPixHit::IsPixHitCorrupted() const {
    if((int)GetPixFlag()) return true;
    return false;
}

unsigned R3BPixHit::GetColumn() const {
    if((fFlag == AlpidePixFlag::kBAD_ADDRESS) || (fFlag == AlpidePixFlag::kBAD_DCOLID))
        cerr << "R3BPixHit::GetColumn() - Warning, return value probably meaningless" << endl;
    unsigned column = fDcol * 2;
    int leftRight = (((fAddress%4)==1 || ((fAddress%4)==2)) ? 1 : 0);
    column += leftRight;
    return column;
}

unsigned R3BPixHit::GetRow() const {
    if(fFlag == AlpidePixFlag::kBAD_ADDRESS)
        cerr << "R3BPixHit::GetRow() - Warning, return value probably meaningless" << endl;
    unsigned row = fAddress / 2; // This is OK for the top-right and the bottom-left pixel within a group of 4
    if((fAddress % 4) == 3) row -=1;
    if((fAddress % 4) == 0) row +=1;
    return row;
}

void R3BPixHit::DumpPixHit() {
	cout << "Board: " << fBoardIndex << " ";
	cout << "| Time: 0x" << std::hex << GetTriggerTime() << " " << std::dec;
	cout << "| Chip: " << fChipId << " ";
	cout << "| Region: " << fRegion << " ";
	cout << "| DCol: " << fDcol << " ";
	cout << "| Address: " << fAddress << " " << endl;
}

