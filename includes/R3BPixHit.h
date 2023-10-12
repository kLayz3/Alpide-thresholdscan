#ifndef R3B_PIXHIT_H
#define R3B_PIXHIT_H

 /* >> Martin Bajzek
 * The "hit" (i.e. responding) pixel is identified thanks to the following coordinates:
 * chip id, double column id, index (address) of the pixel in the double
 * column. The region id is also stored.
 * The container also store the board index and the board receiver id used to
 * collect the data from the chip. The device id (typically used to store the 
 * ladder id) is also stored.
 * Sanity checks are always run on the validity of the chip id, the region id, the
 * double column id and the address. They are used to put a quality flag on the pixel 
 * hit. If the pixel hit is bad in many ways, only one of them can be stored since
 * the flag can only be chosen among a simple enum. */

#include "Common.h"
#include <memory>
#include <stdint.h>

enum class AlpidePixFlag {
	kOK = 0 ,
	kBAD_CHIPID = 1,
	kBAD_REGIONID = 2,
	kBAD_DCOLID = 3,
	kBAD_ADDRESS = 4,
	kSTUCK = 5,
	kDEAD = 6,
	kINEFFICIENT = 7,
	kHOT = 8,
	kUNKNOWN = 9
};

class R3BPixHit {
	uint32_t fBoardIndex;   // index of the MOSAIC board which the chip belongs to
    uint32_t fChipId;       // id of the chip to which belong the hit pixel (!= 15)
    uint32_t fRegion;       // region id in the range [0, 31]
    uint32_t fDcol;         // double column id in the range [0, 511]
    uint32_t fAddress;      // index (address) of the pixel in the double column in the range [0, 1023]
    uint32_t fBunchCounter; // bunch crossing counter, from the chip
	AlpidePixFlag fFlag;       // flag to check the status of this hit pixel

    // trigger time from rataclock
    uint32_t thi;
    uint32_t tlo;

    // illegal chip id, used for initialization
    static const uint32_t ILLEGAL_CHIP_ID = 15;   // i.e. 0b1111
	static const uint32_t ILLEGAL_REGION  = 2048;
	static const uint32_t ILLEGAL_ADDRESS = 4096;

public:
    R3BPixHit();
    R3BPixHit(const R3BPixHit&); //non-default
    R3BPixHit& operator=(const R3BPixHit& obj);

    void SetChipId(const uint32_t value);
    void SetRegion(const uint32_t value);
    void SetDoubleColumn(const uint32_t encoder_id);
    void SetAddress(const uint32_t value);

    inline void SetPixFlag(AlpidePixFlag flag) {fFlag = flag;}
    inline void SetBunchCounter(const uint32_t value) {fBunchCounter = value;};
    inline void SetTriggerTime(const uint32_t thi, const uint32_t tlo) {this->thi=thi; this->tlo=tlo;}

    uint32_t GetChipId() const;
    uint32_t GetRegion() const;
    uint32_t GetAddress() const;
    uint32_t GetDoubleColumn() const;
    uint32_t GetColumn() const;
    uint32_t GetRow() const;
    AlpidePixFlag GetPixFlag() const;
    inline uint32_t GetBunchCounter() const {return fBunchCounter;}
    inline uint64_t GetTriggerTime() const {return (((uint64_t)thi) << 32 | (uint64_t)tlo);}
    bool IsPixHitCorrupted() const;
    
    void DumpPixHit();
	friend class R3BStorePixHit;
};

#endif
