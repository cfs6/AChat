#include "ConfigFileReader.h"
#include "AsyncLog.h"
ConfigFileReader::ConfigFileReader(const char* filename)
								  :loadSucc(false), configFile("")
{
	loadFile(filename);
}

ConfigFileReader::~ConfigFileReader()
{
}

char* ConfigFileReader::getConfigName(const char* name)
{
	if (!loadSucc)
		return nullptr;

	char* value = nullptr;
	auto it = configMap.find(name);
	if (it != configMap.end()) {
		value = (char*)it->second.c_str();
	}

	return value;
}

int ConfigFileReader::setConfigValue(const char* name, const char* value)
{
    if(!loadSucc)
        return -1;

    auto it = configMap.find(name);
    if(it != configMap.end())
    {
        it->second = value;
    }
    else
    {
    	configMap.insert(make_pair(name, value));
    }
    return writeFile();
}

void ConfigFileReader::loadFile(const char* filename)
{
    configFile.clear();
    configFile.append(filename);
	FILE* fp = fopen(filename, "r");
	if (!fp)
	{
		LOGE("can not open %s,errno = %d", filename, errno);
		return;
	}

	char buf[256];
	for (;;)
	{
		char* p = fgets(buf, 256, fp);
		if (!p)
			break;

		size_t len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = 0;			// remove \n at the end

		char* ch = strchr(buf, '#');	// remove string start with #
		if (ch)
			*ch = 0;

		if (strlen(buf) == 0)
			continue;

		parseLine(buf);
	}

	fclose(fp);
	loadSucc = true;
}

int ConfigFileReader::writeFile(const char* filename)
{
   FILE* fp = NULL;
   if(filename == NULL)
   {
       fp = fopen(configFile.c_str(), "w");
   }
   else
   {
       fp = fopen(filename, "w");
   }
   if(fp == NULL)
   {
       return -1;
   }

   char szPaire[128];
   map<string, string>::iterator it = configMap.begin();
   for (; it != configMap.end(); it++)
   {
       memset(szPaire, 0, sizeof(szPaire));
       snprintf(szPaire, sizeof(szPaire), "%s=%s\n", it->first.c_str(), it->second.c_str());
      uint32_t ret =  fwrite(szPaire, strlen(szPaire),1,fp);
      if(ret != 1)
      {
          fclose(fp);
          return -1;
      }
   }
   fclose(fp);
   return 0;
}

void ConfigFileReader::parseLine(char* line)
{
	char* p = strchr(line, '=');
	if (p == NULL)
		return;

	*p = 0;
	char* key =  trimSpace(line);
	char* value = trimSpace(p + 1);
	if (key && value)
	{
		configMap.insert(make_pair(key, value));
	}
}

char* ConfigFileReader::trimSpace(char* name)
{
	// remove starting space or tab
	char* start_pos = name;
	while ( (*start_pos == ' ') || (*start_pos == '\t') )
	{
		start_pos++;
	}

	if (strlen(start_pos) == 0)
		return NULL;

	// remove ending space or tab
	char* end_pos = name + strlen(name) - 1;
	while ( (*end_pos == ' ') || (*end_pos == '\t') )
	{
		*end_pos = 0;
		end_pos--;
	}

	int len = (int)(end_pos - start_pos) + 1;
	if (len <= 0)
		return NULL;

	return start_pos;
}










