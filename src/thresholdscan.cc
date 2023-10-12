#include "libs.hh"
#include "CMDLineParser.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::milliseconds;
using namespace std;

#define timeNow() std::chrono::high_resolution_clock::now()

const std::string _help = 
"\
Run with -h flag to print this message.\n\
";

constexpr int CHARGE_START    = 0;
constexpr int CHARGE_STOP     = 100;
constexpr int N_STEPS         = 50;
constexpr int N_TRIGS_READOUT = 10;
constexpr int N_TRIGS_SEND    = 10;
constexpr int BUFFER_SIZE     = 1 << 24; /* 16 MB */
constexpr int MAX_ROWS        = 512;

typedef shared_ptr<TBoardConfig> TBoardConfig_p;
typedef shared_ptr<TBoardConfigMOSAIC> TBoardConfigMOSAIC_p;
typedef shared_ptr<TReadoutBoard> TReadoutBoard_p;
typedef shared_ptr<TReadoutBoardMOSAIC> TReadoutBoardMOSAIC_p;
typedef shared_ptr<TChipConfig> TChipConfig_p;
typedef shared_ptr<TAlpide> TAlpide_p;
typedef shared_ptr<TSetup> TSetup_p;
typedef shared_ptr<TDevice> TDevice_p;
typedef shared_ptr<TScanConfig> TScanConfig_p;
typedef shared_ptr<TDeviceThresholdScan> TDeviceThresholdScan_p;

bool IsCmdArg(const char*, int, char**);
bool ParseCmdLine(const char* line, std::string&, int, char**);

enum class DebugVerbosity {
	kQUIET  = 0,
	kCHATTY = 10,
};

// Default verbosity:
#if 0
DebugVerbosity verbosity = DebugVerbosity::kCHATTY;
#else 
DebugVerbosity verbosity = DebugVerbosity::kQUIET;
#endif

auto main(int argc, char* argv[]) -> int {
	auto t1 = timeNow();

	if(IsCmdArg("h", argc, argv) || IsCmdArg("help", argc, argv)) {
		cout << _help << endl; return 0;
	}
	if(IsCmdArg("v", argc, argv)) {
		verbosity = DebugVerbosity::kCHATTY;
		printf("Parsed -v flag.\n");
	}

	std::string file_name;
	if(!ParseCmdLine("cfg", file_name, argc, argv)) 
		file_name = "sensors.json";

	std::ifstream f(file_name);
	json data; f >> data;

	for(auto& [boardIP, chipData] : data.items()) {
		TSetup_p s = make_shared<TSetup>();
		s->ReadConfigFile(CONFIG_PATH "/MASTER.cfg");
		s->SetDeviceAddress(boardIP.c_str());
		s->InitializeSetup();
		TDevice_p device = s->GetDevice();

		/* Set proper receiverId's */
		/* Put the json array into (chipId,recId) map */
		std::unordered_map<int,int> recMap = std::invoke(
			[&](){
				unordered_map<int,int> recMap;
				for(auto& [_k, _chipData] : chipData.items()) {
					int k = _chipData["chipId"].get<int>();
					int v = _chipData["recId"].get<int>();
					recMap.emplace(std::make_pair(k,v));
				}
				return recMap;
			}
		);

		for(int i=0; i < device->GetNWorkingChips(); ++i) {
			device->GetChip(i)->ActivateConfigMode();
			int chipId = device->GetChipId(i);
			auto config = device->GetChipConfig(i);
			config->SetParamValue("RECEIVER", recMap[chipId]);
			
			device->GetChip(i)->BaseConfig();
			device->GetChip(i)->ActivateReadoutMode();
		}
		
		/* Device object is built - with proper receiver map */
		TScanConfig_p scan_config = make_shared<TScanConfig>();
		TDeviceThresholdScan_p main_scan = make_shared<TDeviceThresholdScan>(device, scan_config);
		
		main_scan->SetPrefixFilename("threshold_scan_");
		main_scan->Init();
		usleep(500);
		main_scan->Go();
		main_scan->Terminate();

		main_scan->WriteDataToFile();
	}

	auto t2 = timeNow();
    cout << "\nTime taken: " << duration_cast<seconds>(t2-t1).count() << "s\n";
	return 0;
}
