#include "R3BStorePixHit.h"
#include "R3BPixHit.h"
#include "R3BAlpideDecoder.h"
#include "Common.h"
#include <stdexcept>
#include <iostream>
#include <climits>
#include <string.h>
#include "TFile.h"
#include "TTree.h"
using namespace std;

R3BStorePixHit::R3BStorePixHit() : 
	fTree(nullptr),
	fFile(nullptr),
	fSuccessfulInit(false),
	fNEntriesAutoSave(10000) {
		fData.boardIndex = UINT_MAX;
		fData.chipId = UINT_MAX;
		fData.row = 0;
		fData.col = 0;
		fData.thi = 0;
		fData.tlo = 0;
	}

R3BStorePixHit::~R3BStorePixHit() {
	if(fFile) delete fFile;
    if(fTree) delete fTree;
}

void R3BStorePixHit::SetCyclicAutoSave(long nEntries) {
    if(nEntries <= 0) return;
    fNEntriesAutoSave = nEntries;
}

void R3BStorePixHit::Init() {
    if(fOutFileName.empty())  {
        throw runtime_error("R3BStorePixHit::Init() - empty output name! Please use SetFileName() first.");
    }
    if(!fFile || fFile->IsZombie()) {
        try {
			fFile = new TFile(fOutFileName.c_str(), "RECREATE");
		}
		catch(exception& msg) {
            cerr << msg.what() << endl;
            exit(EXIT_FAILURE);
        }
    }
	/* The object completely owns the fTree and fFile memory */
    if(!fTree) {
        fTree = new TTree("PixTree", "PixTree");
		fTree->Branch("T_HI", &fData.thi);
		fTree->Branch("T_LO", &fData.tlo);
		fTree->Branch("CHIP_ID", &fData.chipId);
		fTree->Branch("ROW", &fData.row);
		fTree->Branch("COL", &fData.col);
        fTree->SetAutoSave(fNEntriesAutoSave); // flush the TTree to disk every N entries
        fTree->SetDirectory(fFile);
        fTree->SetImplicitMT(true);
    }
    fSuccessfulInit = true;
}

void R3BStorePixHit::Fill(const R3BPixHit& hit) {
    if(!IsInitOk())
        throw runtime_error("R3BStorePixHit::Fill() - object not (successfully) initialized! Please use Init() first.");
    if(!fFile || fFile->IsZombie()) {
        fSuccessfulInit = false;
        throw runtime_error("R3BStorePixHit::Fill() - no viable output file! Please use Init() first.");
    }
    if(!fTree || fTree->IsZombie()) {
        fSuccessfulInit = false;
        throw runtime_error("R3BStorePixHit::Fill() - no TTree! Please use Init() first.");
    }
	SetDataSummary(hit);
	fTree->Fill();
}

/* Ideally R3BStorePixHit should move the data from decoder.fHits vector */
/* Right now, one copy is redundant */
void R3BStorePixHit::Fill(const R3BAlpideDecoder& decoder) {
	if(!IsInitOk())
		throw runtime_error("R3BStorePixHit::Fill() - object not (successfully) initialized! Please use Init() first.");
	if(!fFile || fFile->IsZombie()) {
        fSuccessfulInit = false;
        throw runtime_error("R3BStorePixHit::Fill() - no viable output file! Please use Init() first.");
    }
    if(!fTree || fTree->IsZombie()) {
        fSuccessfulInit = false;
        throw runtime_error("R3BStorePixHit::Fill() - no TTree! Please use Init() first.");
    }
	for(const auto& hit : decoder.fHits) {
		SetDataSummary(hit);
		fTree->Fill();
	}
}

void R3BStorePixHit::Terminate() {
    fTree->Write();
	fFile->Write();
    fFile->Close();
}

void R3BStorePixHit::SetDataSummary(const R3BPixHit& hit) {
	/* TODO:
	 * Implement reader for thi & tlo 
	 * */
    fData.chipId   = hit.GetChipId();
    fData.row	   = hit.GetRow();
    fData.col      = hit.GetColumn();
}
