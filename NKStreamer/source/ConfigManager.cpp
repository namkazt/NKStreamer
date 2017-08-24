
#include "ConfigManager.h"
#include "UIHelper.h"

#define CONFIG_FILE_NAME "nkstreamer.cfg"

static ConfigManager* ref;

ConfigManager* ConfigManager::R()
{
	if (ref == nullptr) ref = new ConfigManager();
	return ref;
}

ConfigManager::ConfigManager()
{
}

void ConfigManager::InitConfig()
{
	config_init(&_config);
	if (!config_read_file(&_config, CONFIG_FILE_NAME))
	{
		/*std::fstream cfgFileStream;
		cfgFileStream.open(CONFIG_FILE_NAME, std::fstream::in | std::fstream::out | std::fstream::trunc);
		cfgFileStream.close();*/
		//// read again
		//config_read_file(&_config, cfgFileName);

		UIHelper::R()->AddLog("Can't find config file.");
		UIHelper::R()->AddLog("Creating new config file...");

		config_setting_t *root, *setting;
		root = config_root_setting(&_config);
		setting = config_setting_add(root, "ip", CONFIG_TYPE_STRING);
		config_setting_set_string(setting, "192.168.31.222");
		setting = config_setting_add(root, "port", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 1234);
		setting = config_setting_add(root, "video_quality", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 40);
		setting = config_setting_add(root, "video_mode", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 0);
		setting = config_setting_add(root, "thread_num", CONFIG_TYPE_INT);
		config_setting_set_int(setting, 3);

		config_set_options(&_config, 0);
		config_write_file(&_config, CONFIG_FILE_NAME);

		_cfg_ip = "192.168.31.222";
		_cfg_port = 1234;
		_cfg_video_quality = 40;
		_cfg_video_mode = 0;
		_cfg_thread_num = 3;

		return;
	}
	if(!config_lookup_string(&_config, "ip", &_cfg_ip))
	{
		UIHelper::R()->AddLog("Missing properties: ip -> set default");
		_cfg_ip = "192.168.31.222";
	}
	if (!config_lookup_int(&_config, "port", &_cfg_port))
	{
		UIHelper::R()->AddLog("Missing properties: port -> set default");
		_cfg_port = 1234;
	}
	if (!config_lookup_int(&_config, "video_quality", &_cfg_video_quality))
	{
		UIHelper::R()->AddLog("Missing properties: video_quality -> set default");
		_cfg_video_quality = 40;
	}
	if (!config_lookup_int(&_config, "video_mode", &_cfg_video_mode))
	{
		UIHelper::R()->AddLog("Missing properties: video_mode -> set default");
		_cfg_video_mode = 0;
	}
	if (!config_lookup_int(&_config, "thread_num", &_cfg_thread_num))
	{
		UIHelper::R()->AddLog("Missing properties: thread_num -> set default");
		_cfg_thread_num = 3;
	}
}

void ConfigManager::Save()
{
	config_setting_t *root, *setting;
	root = config_root_setting(&_config);

	setting = config_setting_get_member(root, "ip");
	config_setting_set_string(setting, std::string(_cfg_ip).c_str());
	setting = config_setting_get_member(root, "port");
	config_setting_set_int(setting, _cfg_port);
	setting = config_setting_get_member(root, "video_quality");
	config_setting_set_int(setting, _cfg_video_quality);
	setting = config_setting_get_member(root, "video_mode");
	config_setting_set_int(setting, _cfg_video_mode);
	setting = config_setting_get_member(root, "thread_num");
	config_setting_set_int(setting, _cfg_thread_num);

	UIHelper::R()->AddLog("Save config success");

	config_write_file(&_config, CONFIG_FILE_NAME);
}

void ConfigManager::Destroy()
{
	config_destroy(&_config);
}
