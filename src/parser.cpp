/* Copyright (C) 2002  Kari Pahula
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if defined(_MSC_VER)
	#pragma warning(disable : 4786)
#endif

#include "version.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include "parser.h"
#include "util.h"
#include <yaml-cpp/yaml.h>

// TODO: Move overscan setting and toInt() to this file.
#include "input.h"
int toInt(const std::string& s);

using namespace std;

cfg_store cfg;

void cfg_store::apply_setting(const std::string& category, const std::string& setting, const std::string& value)
{
    data[category][setting] = value;
}

std::string cfg_store::get_setting(const std::string& category, const std::string& setting)
{
	map<string, map<string, string> >::iterator a1 = data.find(category);
	if(a1 != data.end())
	{
		map<string, string>::iterator a2 = a1->second.find(setting);
		if(a2 != a1->second.end())
			return a2->second;
	}
	
	return "";
}

bool cfg_store::load_settings()
{
    // Load defaults
    apply_setting("", "version", "1");
    apply_setting("sound", "sound", "on");
    
    apply_setting("graphics", "render", "normal");
    apply_setting("graphics", "fullscreen", "off");
    apply_setting("graphics", "overscan_percentage", "0");
    
    apply_setting("effects", "gore", "on");
    apply_setting("effects", "mini_hp_bar", "on");
    apply_setting("effects", "hit_flash", "on");
    apply_setting("effects", "hit_recoil", "off");
    apply_setting("effects", "attack_lunge", "on");
    apply_setting("effects", "hit_anim", "on");
    apply_setting("effects", "damage_numbers", "off");
    apply_setting("effects", "heal_numbers", "on");
    
    Log("Loading settings\n");
    SDL_RWops* rwops = open_read_file("cfg/openglad.yaml");
    if(rwops == NULL)
	{
		Log("Could not open config file. Using defaults.");
		return false;
	}
    
	char* config = read_rest_of_file(rwops);
    SDL_RWclose(rwops);
	YAML::Node node = YAML::Load(config);
	delete[] config;
    
    std::string last_scalar;
    std::string current_category;

	for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
	{
		if (!it->second.IsMap()) continue;

		for (YAML::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
		{
			apply_setting(it->first.as<std::string>(), jt->first.as<std::string>(), jt->second.as<std::string>());
		}
	}
    
    // Update game stuff from these settings
    overscan_percentage = toInt(get_setting("graphics", "overscan_percentage"))/100.0f;
    update_overscan_setting();
    
	return true;
}


bool cfg_store::save_settings()
{
    char buf[40];
    snprintf(buf, 40, "%.0f", 100*overscan_percentage);
    apply_setting("graphics", "overscan_percentage", buf);
    
    SDL_RWops* outfile = open_write_file("cfg/openglad.yaml");
    if(outfile == NULL)
    {
        Log("Couldn't open cfg/openglad.yaml for writing.\n");
        return false;
    }

	Log("Saving settings\n");
	
	YAML::Emitter out;
	
	// Each category is a mapping that holds setting/value pairs
	for(auto e = data.begin(); e != data.end(); ++e)
	{
		if(e->first.size() > 0)
		{
			out << e->first.c_str() << YAML::BeginMap;
		}
		
		for(auto f = e->second.begin(); f != e->second.end(); f++)
		{
			out << YAML::Key << f->first.c_str() << YAML::Value << f->second.c_str();
		}
		
		if(e->first.size() > 0)
		{
			out << YAML::EndMap;
		}
	}
	
	SDL_RWwrite(outfile, out.c_str(), 1, out.size());
	SDL_RWclose(outfile);
	
	return true;
}

void cfg_store::commandline(int &argc, char **&argv)
{
	const char helpmsg[] = 
"Usage: openglad [-d -f ...]\n"
"  -s		Turn sound on\n"
"  -S		Turn sound off\n"
"  -n		Run at 320x200 resolution\n"
"  -d		Double pixel size\n"
"  -e		Use eagle engine for pixel doubling\n"
"  -i		Use sai2x engine for pixel doubling\n"
"  -f		Use full screen\n"
"  -h		Print a summary of the options\n"
"  -v		Print the version number\n";

	const char versmsg[] = "openglad version " OPENGLAD_VERSION_STRING "\n";

	// Begin changes by David Storey (Deathifier)
	// FIX: Handle mutually exclusive arguments being used at the same time?
	// E.G. -s and -S

	// Iterate over arguments, ignoring the first (Program Name).
	for(int argnum = 1; argnum < argc; argnum++)
	{
		// Look for arguments of 2 chars only:
		if(argv[argnum][0] == '-' && strlen(argv[argnum]) == 2)
		{
			// To handle arguments which have aditional arguments attached
			// to them, take care of it within the case statement and
			// increment argnum appropriately.
			switch(argv[argnum][1])
			{
				case 'h':
					Log("%s", helpmsg);
					exit (0);
				case 'v':
					Log("%s", versmsg);
					exit (0);
				case 's':
					data["sound"]["sound"] = "on";
					Log("Sound is on.");
					break;
				case 'S':
					data["sound"]["sound"] = "off";
					Log("Sound is off.");
					break;
				case 'n':
					data["graphics"]["render"] = "normal";
					Log("Screen Resolution set to 320x200.");
					break;
				case 'd':
					data["graphics"]["render"] = "double";
					Log("Screen Resolution set to 640x400 (basic mode).");
					break;
				case 'e':
					data["graphics"]["render"] = "eagle";
					Log("Screen Resolution set to 640x400 (eagle mode).");
					break;
				case 'x':
					data["graphics"]["render"] = "sai";
					Log("Screen Resolution set to 640x400 (sai2x mode).");
					break;
				case 'f':
					data["graphics"]["fullscreen"] = "on";
					Log("Running in fullscreen mode.");
					break;
				default:
					Log("Unknown argument %s ignored.", argv[argnum]);
			}
		}
	}

	// End Changes
}


bool cfg_store::is_on(const std::string& category, const std::string& setting)
{
    return get_setting(category, setting) == "on";
}

