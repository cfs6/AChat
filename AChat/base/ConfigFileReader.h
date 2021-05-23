#ifndef CONFIGFILEREADER_H_
#define CONFIGFILEREADER_H_

#include "util.h"
#include <map>
using namespace std;
class ConfigFileReader
{
public:
	ConfigFileReader(const char* fileName);
	~ConfigFileReader();

    char* getConfigName(const char* name);
    int setConfigValue(const char* name, const char*  value);

private:
    void loadFile(const char* filename);
    int writeFile(const char*filename = NULL);
    void parseLine(char* line);
    char* trimSpace(char* name);
private:
	bool                  loadSucc;
	map<string, string>   configMap;
	string                configFile;
};


#endif /* CONFIGFILEREADER_H_ */
