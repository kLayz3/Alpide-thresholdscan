#include <cstdio>
#include <regex>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>

using namespace std;

bool IsCmdArg(const char* line, int argc, char** argv) {
	cmatch m;
    std::regex r(R"(^(?:-){1,2}(\w+)$)");
    for(int i(1); i<argc; ++i) 
		if(regex_match(argv[i],m,r) && !strcmp(m[1].str().c_str(),str)) return 1;
    return 0;
}

/* Cmd args have to be: --tag0=identifier0 --tag1=identifier1 ... */
bool ParseCmdLine(const char* line, string& parsed, int argc, char** argv) {
    cmatch m;
    std::regex r(R"(^(?:-){0,2}([^=]+)[=](.+)$)");
    for(int i(1); i<argc; ++i) {
        if(regex_match(argv[i], m, r) && !strcmp(m[1].str().c_str(), line)) {
            parsed = m[2].str(); return 1;
		}
    }   
    return 0;
}

void RemoveCharsFromString(string& str, string chars) {
	for(char c : chars) {
		str.erase(std::remove(str.begin(), str.end(), c), str.end());
	}
}

vector<string> SplitStringToVector(const string& str, char delim) {
	std::stringstream iss(str);
	vector<string> parts;
	string part;
	while(std::getline(iss, part, delim)) {
		parts.push_back(part);
	}
	return parts;
}

unordered_set<string> SplitStringToSet(const string& str, char delim) {
	std::stringstream iss(str);
	unordered_set<string> parts;
	string part;
	while(std::getline(iss, part, delim)) {
		parts.insert(part);
	}
	return parts;
}





