#include "R3BAlpideDecoder.h"
#include "R3BStorePixHit.h"
#include "R3BPixHit.h"
#include "THisto.h"
#include "TErrorCounter.h"
#include <stdint.h>
#include <iostream>
#include <string>
#include <climits>
#include <stdexcept>

using namespace std;

R3BAlpideDecoder::R3BAlpideDecoder() : 
    fNewEvent(false),
	fBoardIndex(UINT_MAX),
    fChipId(UINT_MAX),
    fRegion(32),
    fFlags(0),
	thi(UINT_MAX),
	tlo(UINT_MAX),
    fDataType(AlpideDataType::kUNKNOWN) {
		fHits.reserve(2048);
	}

/* MARK: Main method */
bool R3BAlpideDecoder::DecodeEvent(unsigned char* data, int nBytes) {
    fFlags  = 0;
    fChipId = -1;
    fRegion = 32; // bad region 
    fDataType = AlpideDataType::kUNKNOWN;

    bool started = false;  // event has started, i.e. chip header has been found
    bool finished = false; // event trailer found
    bool corrupt  = false; // corrupt data found (i.e. data without region or chip)
    int byte = 0;
    
    unsigned char last = 0x0;
    
    while(byte < nBytes) {
        last = data[byte];
        FindDataType(data[byte]);
        
        switch(fDataType) {
            case AlpideDataType::kIDLE:
                byte += GetWordLength();
                break;
            case AlpideDataType::kBUSYON:
                byte += GetWordLength();
                break;
            case AlpideDataType::kBUSYOFF:
                byte += GetWordLength();
                break;
            case AlpideDataType::kEMPTYFRAME:
                started = true;
                DecodeEmptyFrame(data + byte);
                byte += GetWordLength();
                finished = true;
                break;
            case AlpideDataType::kCHIPHEADER:
                started = true;
                finished = false;
                DecodeChipHeader(data + byte);
                byte += GetWordLength();
                break;
            case AlpideDataType::kCHIPTRAILER:
                if(!started) {
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Error: chip trailer found before chip header ?" << endl;
                    return false;
                }
                if(finished) {
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Error: chip trailer found after event was finished ?" << endl;
                    return false;
                }
                DecodeChipTrailer(data + byte);
                finished = true;
                fChipId = -1;
                byte += GetWordLength();
                break;
            case AlpideDataType::kREGIONHEADER:
                if(!started) {
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Error: region header found before chip header or after chip trailer" << endl;
                    return false;
                }
                DecodeRegionHeader(data + byte);
                byte += GetWordLength();
                break;
            case AlpideDataType::kDATASHORT:
                if(!started) {
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Error: hit data found before chip header or after chip trailer" << endl;
                    return false;
                }
                if(fRegion == 32)
                    cout << "R3BAlpideDecoder::DecodeEvent() - Warning: data word without region (Chip " << fChipId << ")" << endl;
				corrupt = DecodeDataShort(data + byte);
				byte += GetWordLength();
				break;
            case AlpideDataType::kDATALONG:
                if(!started) {
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Error: hit data found before chip header or after chip trailer" << endl;
                    return false;
                }
                if(fRegion == 32)
                    cerr << "R3BAlpideDecoder::DecodeEvent() - Warning: data word without region, skipping (Chip " << fChipId << ")" << endl;
                corrupt = DecodeDataLong(data + byte);
                byte += GetWordLength();
                break;
            case AlpideDataType::kUNKNOWN:
                cerr << "R3BAlpideDecoder::DecodeEvent() - Error: data of unknown type 0x" << std::hex << data[byte] << std::dec << endl;
                return false;
        }
    }
	if(started && !finished) {
		cout << "R3BAlpideDecoder::DecodeEvent() - Warning (chip "<< fChipId << "): event not finished at end of data, last byte was 0x" << std::hex << (int) last << std::dec << ", event length = " << nBytes << endl;
		return false;
	}
	else if(!started) {
		cout << "R3BAlpideDecoder::DecodeEvent() - Warning: event not started at end of data." << endl;
		return false;
	}
    return !corrupt;
}

void R3BAlpideDecoder::FindDataType(unsigned char dataWord) {
    if     (dataWord == 0xff)          fDataType = AlpideDataType::kIDLE;
    else if(dataWord == 0xf1)          fDataType = AlpideDataType::kBUSYON;
    else if(dataWord == 0xf0)          fDataType = AlpideDataType::kBUSYOFF;
    else if((dataWord & 0xf0) == 0xa0) fDataType = AlpideDataType::kCHIPHEADER;
    else if((dataWord & 0xf0) == 0xb0) fDataType = AlpideDataType::kCHIPTRAILER;
    else if((dataWord & 0xf0) == 0xe0) fDataType = AlpideDataType::kEMPTYFRAME;
    else if((dataWord & 0xe0) == 0xc0) fDataType = AlpideDataType::kREGIONHEADER;
    else if((dataWord & 0xc0) == 0x40) fDataType = AlpideDataType::kDATASHORT;
    else if((dataWord & 0xc0) == 0x00) fDataType = AlpideDataType::kDATALONG;
    else fDataType = AlpideDataType::kUNKNOWN;
}

int R3BAlpideDecoder::GetWordLength() const {
	if(fDataType == AlpideDataType::kDATALONG) return 3;
	else if((fDataType == AlpideDataType::kDATASHORT) ||
			(fDataType == AlpideDataType::kCHIPHEADER) || 
			(fDataType == AlpideDataType::kEMPTYFRAME))
		return 2;
	else return 1;
}

/* 16-bits: 1010 <chip_id[3:0]> <bunch_counter[10:3]> 1011 */
void R3BAlpideDecoder::DecodeChipHeader(unsigned char* data) {
  fChipId = (uint32_t)(*data & 0xf); 
  fNewEvent = true;
}

/* 8-bits: 1011 <readout_flags[3:0]> */
void R3BAlpideDecoder::DecodeChipTrailer(unsigned char* data) {
	fFlags = data[0] & 0xf;
}

/* 8-bits: 110 <region_id[4:0]> */
void R3BAlpideDecoder::DecodeRegionHeader(unsigned char* data) {
    fRegion = data[0] & 0x1f;
    fNewEvent = false;
}

/* 16-bits: 1110 <chip_id[3:0]> <bunch_counter[10:3]> */
void R3BAlpideDecoder::DecodeEmptyFrame(unsigned char* data) {
  fChipId = (uint32_t)(*data & 0xf); 
}

/* 16-bits: 01 <encoder_id[3:0]> <addr[9:0]> */
bool R3BAlpideDecoder::DecodeDataShort(unsigned char* data) {
	R3BPixHit hit;
	hit.SetTriggerTime(thi, tlo);

    uint16_t data_field = (((uint16_t) data[0]) << 8) | (uint16_t)data[1];

    bool corrupt = false;
    hit.SetChipId(fChipId); // only basic checks on chip id done here
    hit.SetRegion(fRegion); // can generate a bad region flag

	uint32_t encoder_id = (data_field & 0x3c00) >> 10;
    hit.SetDoubleColumn(encoder_id); // can generate a bad dcol flag

	uint32_t address = (data_field & 0x03ff);		
	hit.SetAddress(address);

	// check if there are duplicates in the vector
	if((fHits.size() > 0) && !fNewEvent) {
		if((hit.GetRegion() == (fHits.back()).GetRegion())
				&& (hit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
				&& (hit.GetAddress() == (fHits.back()).GetAddress())) {
			cerr << "R3BAlpideDecoder::DecodeDataWordShort() - received a pixel twice." << endl;
			hit.SetPixFlag(AlpidePixFlag::kSTUCK);
			(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
			cerr << "\t -- current hit pixel :" << endl;
			hit.DumpPixHit();
			cerr << "\t -- previous hit pixel :" << endl;
			(fHits.back()).DumpPixHit();
		}
		else if((hit.GetRegion() == (fHits.back()).GetRegion())
				&& (hit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
				&& (hit.GetAddress() < (fHits.back()).GetAddress())) {
			cerr << "R3BAlpideDecoder::DecodeDataWordShort() - address of pixel is lower than previous one in same double column." << endl;
			hit.SetPixFlag(AlpidePixFlag::kSTUCK);
			(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
			cerr << "\t -- current hit pixel :" << endl;
			hit.DumpPixHit();
			cerr << "\t -- previous hit pixel :" << endl;
			(fHits.back()).DumpPixHit();
		}
	}
	if(hit.GetPixFlag() == AlpidePixFlag::kUNKNOWN) { // nothing bad detected, it means that the flag still has its initialization value => the pixel hit is ok
		hit.SetPixFlag(AlpidePixFlag::kOK);
	}
	// data word is corrupted if there is any bad hit found
	corrupt = corrupt | hit.IsPixHitCorrupted();

	//finally flush the decoded hit into fHits vector
	fHits.emplace_back(std::move(hit));
	return corrupt;
}

bool R3BAlpideDecoder::DecodeDataLong(unsigned char* data) {
    R3BPixHit hit;
    hit.SetTriggerTime(thi, tlo);
    
    uint16_t data_field = (((uint16_t) data[0]) << 8) | (uint16_t)data[1];

    bool corrupt = false;

    hit.SetChipId(fChipId); // only basic checks on chip id done here
    hit.SetRegion(fRegion); // can generate a bad region flag
    hit.SetDoubleColumn((data_field & 0x3c00) >> 10); // calculated from encoder_id & region
    
    uint32_t address = (data_field & 0x03ff);

    for(int i=-1; i<7; ++i) {
        if((i >= 0) && (!((data[2] >> i) & 0x1))) continue;
        R3BPixHit singleHit(hit); // i=0 is the first pixel hit in the cluster

        // set hit address on the new hit, can generate a bad address flag
        singleHit.SetAddress(address + (i + 1));

        // check if pixel deserves a stuck flag
        if((fHits.size() > 0) && !fNewEvent) {
            if((singleHit.GetRegion() == (fHits.back()).GetRegion())
            && (singleHit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
			&& (singleHit.GetAddress() == (fHits.back()).GetAddress())) {
				cerr << "R3BAlpideDecoder::DecodeDataWord() - received a pixel twice." << endl;
				singleHit.SetPixFlag(AlpidePixFlag::kSTUCK);
				(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
				cerr << "\t -- current hit pixel :" << endl;
				singleHit.DumpPixHit();
				cerr << "\t -- previous hit pixel :" << endl;
				(fHits.back()).DumpPixHit();
			}
            else if((singleHit.GetRegion() == (fHits.back()).GetRegion())
			&& (singleHit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
			&& (singleHit.GetAddress() < (fHits.back()).GetAddress())) {
				cerr << "R3BAlpideDecoder::DecodeDataWord() - address of pixel is lower than previous one in same double column." << endl;
				singleHit.SetPixFlag(AlpidePixFlag::kSTUCK);
				(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
				cerr << "\t -- current hit pixel :" << endl;
				singleHit.DumpPixHit();
				cerr << "\t -- previous hit pixel :" << endl;
				(fHits.back()).DumpPixHit();
			}
        }
        if(singleHit.GetPixFlag() == AlpidePixFlag::kUNKNOWN) { // nothing bad detected, it means that the flag still has its initialization value => the pixel hit is ok
            singleHit.SetPixFlag(AlpidePixFlag::kOK);
        }
        // data word is corrupted if there is any bad hit found
        corrupt = corrupt | singleHit.IsPixHitCorrupted();

		//finally flush the decoded hit into fHits vector
        fHits.emplace_back(std::move(singleHit));
    }

    fNewEvent = false;
    return corrupt;
}

/*
bool R3BAlpideDecoder::DecodeDataWord(unsigned char* data, bool datalong) {
    R3BPixHit hit();
    hit.SetBunchCounter(fBunchCounter);
    hit.SetTriggerTime(thi, tlo);
    
    uint16_t data_field = (((uint16_t) data[0]) << 8) | (uint16_t)data[1];

    bool corrupt = false;

    hit.SetChipId(fChipId); // only basic checks on chip id done here
    hit.SetRegion(fRegion); // can generate a bad region flag
    hit.SetDoubleColumn((data_field & 0x3c00) >> 10); // can generate a bad dcol flag
    
    uint32_t address = (data_field & 0x03ff);

    int hitmap_length;
    if(datalong) hitmap_length = 7; // clustering enabled
	else hitmap_length = 0;         // clustering disabled, most common

    for(int i=-1; i<hitmap_length; ++i) {
        if((i >= 0) && (!((data[2] >> i) & 0x1))) continue;
        R3BPixHit singleHit(hit);

        // set hit address on the new hit, can generate a bad address flag
        singleHit.SetAddress(address + (i + 1));

        // check if pixel deserves a stuck flag
        if((fHits.size() > 0) && !fNewEvent) {
            if((singleHit.GetRegion() == (fHits.back()).GetRegion())
            && (singleHit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
			&& (singleHit.GetAddress() == (fHits.back()).GetAddress())) {
				cerr << "R3BAlpideDecoder::DecodeDataWord() - received a pixel twice." << endl;
				singleHit.SetPixFlag(AlpidePixFlag::kSTUCK);
				(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
				cerr << "\t -- current hit pixel :" << endl;
				singleHit.DumpPixHit();
				cerr << "\t -- previous hit pixel :" << endl;
				(fHits.back()).DumpPixHit();
				fErrorCounter->IncrementNPrioEncoder(singleHit); 
			}
            else if((singleHit.GetRegion() == (fHits.back()).GetRegion())
			&& (singleHit.GetDoubleColumn() == (fHits.back()).GetDoubleColumn())
			&& (singleHit.GetAddress() < (fHits.back()).GetAddress())) {
				cerr << "R3BAlpideDecoder::DecodeDataWord() - address of pixel is lower than previous one in same double column." << endl;
				singleHit.SetPixFlag(AlpidePixFlag::kSTUCK);
				(fHits.back()).SetPixFlag(AlpidePixFlag::kSTUCK);
				cerr << "\t -- current hit pixel :" << endl;
				singleHit.DumpPixHit();
				cerr << "\t -- previous hit pixel :" << endl;
				(fHits.back()).DumpPixHit();
				fErrorCounter->IncrementNPrioEncoder(singleHit); 
			}
        }
        // check if chip index (board id, receiver id, ladder id, chip id) matches
        // the one currently read on the device
        if(!IsValidChipIndex(singleHit)) {
            cerr << "R3BAlpideDecoder::DecodeDataWord() - found bad chip index, data word = 0x" << std::hex << data_field << std::dec << endl;
            singleHit.SetPixFlag(AlpidePixFlag::kBAD_CHIPID);
            cerr << "\t -- current hit pixel :" << endl;
            singleHit.DumpPixHit();
        }
        if(singleHit.GetPixFlag() == AlpidePixFlag::kUNKNOWN) { // nothing bad detected, it means that the flag still has its initialization value => the pixel hit is ok
            singleHit.SetPixFlag(AlpidePixFlag::kOK);
        }
        // data word is corrupted if there is any bad hit found
        corrupt = corrupt | singleHit.IsPixHitCorrupted();

		//finally flush the decoded hit into fHits vector
        fHits.emplace_back(std::move(singleHit));
    }
    hit.reset();
    fNewEvent = false;
    return corrupt;
}
*/
