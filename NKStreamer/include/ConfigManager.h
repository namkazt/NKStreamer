#pragma once
#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include <3ds.h>
#include "libconfig.h"

class ConfigManager
{
private:
	config_t _config;

public:
	const char* _cfg_ip;
	int _cfg_port;
	int _cfg_video_quality; 
	int _cfg_video_mode;
	int _cfg_thread_num;
public:
	static ConfigManager* R();
	ConfigManager();

	void InitConfig();
	void Save();
	void Destroy();
};


#endif