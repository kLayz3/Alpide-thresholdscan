#ifndef R3B_ALPIDEDECODER_H
#define R3B_ALPIDEDECODER_H

#include <vector>
#include <memory>
#include <stdint.h>
#include <R3BPixHit.h>
#include "Common.h"

enum class AlpideDataType {
	kIDLE,
	kCHIPHEADER,
	kCHIPTRAILER,
	kEMPTYFRAME,
	kREGIONHEADER,
	kDATASHORT,
	kDATALONG,
	kBUSYON,
	kBUSYOFF,
	kUNKNOWN
};

class R3BAlpideDecoder {
	friend class R3BStorePixHit;

    bool fNewEvent;			// allows the decoder to know if the current data corresponds to a new event

	uint32_t fBoardIndex;   // index of the MOSAIC board based on it's unique IP 	
    uint32_t fChipId;		// chip id decoded from chip header
    uint32_t fRegion;		// region id decoded from region header
    uint32_t fFlags;		// flag decoded from chip trailer
    uint32_t tlo;			// Current status of timestamp
    uint32_t thi;			// Current status of timestamp

    AlpideDataType fDataType;    // type of the data word currently being decoded


    // Hit pixel list with all decoded hits for the current event
	// The most important field of the class
    std::vector<R3BPixHit> fHits;

public:
    R3BAlpideDecoder();
	inline void SetBoard(uint32_t board) {fBoardIndex = board;} 
     /* Main method of the class - decode each event read by the readout board */
    bool DecodeEvent(unsigned char* data, int nBytes);
        
private:
    // find the data type of the given data word
    void FindDataType(unsigned char dataWord);
    
    // return the length of the data word depending on its data type
    int GetWordLength() const;
    
    // extract the bunch counter and chip id from a data word of type "chip header"
    void DecodeChipHeader(unsigned char* data);
    
    // extract the flag from a data word of type "chip trailer"
    void DecodeChipTrailer(unsigned char* data);
    
    // extract the region id from a data word of type "region header"
    void DecodeRegionHeader(unsigned char* data);
    
    // extract the bunch counter and chip id from a data word of type "empty frame"
    void DecodeEmptyFrame(unsigned char* data);
   
	bool DecodeDataShort(unsigned char* data);
	bool DecodeDataLong(unsigned char* data);
};

#endif
