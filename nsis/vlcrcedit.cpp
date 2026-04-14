/*****************************************************************************
 * discordipc.h: Discord Rich Presence plugin for VLC
 *****************************************************************************
 * Copyright (C) 2026 Zukaritasu
 *
 * Authors: Zukaritasu <zukaritasu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *****************************************************************************/

#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>

#include <string>
#include <algorithm>

#ifdef _WIN32
	#include <shlobj.h>
	#include <knownfolders.h>
#endif // _WIN32

#define PLUGIN_NAMESPACE "discord_rpc"

#ifdef _WIN32
std::string WstrToUtf8(const std::wstring& wstr) 
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    
    std::string strTo(size_needed, 0);
    
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    
    return strTo;
}
#endif // _WIN32

static std::string trim(std::string s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start)))
        start++;

    auto end = s.end();
    while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1))))
        end--;

    return std::string(start, end);
}

class Property
{
public:
    Property() = default;
    ~Property() = default;

    std::string _key;
    std::string _value;
    std::string _desc;
    bool _disabled;
};

class Module
{
private:
    std::vector<Property> _props;
    std::string _name;
    std::string _desc;

public:
    Module(const std::string &name, const std::string &desc);
    ~Module() = default;

    void add(const Property &prop);
    Property *get(const std::string &name);
    const std::string &get_name();
    const std::string &get_desc();
    const std::vector<Property> &properties();
};

Module::Module(const std::string &name, const std::string &desc)
{
    _name = name;
    _desc = desc;
}

void Module::add(const Property &prop)
{
    _props.push_back(prop);
}

Property *Module::get(const std::string &name)
{
    for (auto &i : _props)
    {
        if (i._key == name)
        {
            return &i;
        }
    }

    return nullptr;
}

const std::string &Module::get_name()
{
    return _name;
}

const std::string &Module::get_desc()
{
    return _desc;
}

const std::vector<Property> &Module::properties()
{
    return _props;
}

class VlcRc
{
private:
    std::vector<Module *> _modules;
    void load_modules(std::ifstream &input_file);

public:
    VlcRc() = default;
    ~VlcRc();

    void add(Module *module);
    void load(const std::string &filepath);
    void save(const std::string &filepath);
    void remove(const std::string &name);
    Module *get(const std::string &name);
    Module *get(int i);
    size_t size();
};

void VlcRc::load_modules(std::ifstream &input_file)
{
    std::string line;
    bool load_desc = true;
    Property p;
    Module *module = nullptr;
    bool first_line = true;

    while (std::getline(input_file, line))
    {
        if (first_line)
        {
            if (line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
            {
                line.erase(0, 3);
            }
            first_line = false;
        }

        line = trim(line);

        if (line.empty())
            continue;
        if (line.compare(0, 2, "##") == 0 || line.compare(0, 3, "###") == 0)
            continue;

        if (line[0] == '[')
        {
            std::smatch matches;
            if (!std::regex_search(line, matches, std::regex(R"(^\[(.*?)\]\s*#\s*(.*)$)")))
            {
                throw std::runtime_error("Syntax error in module header: " + line);
            }

            module = new Module(trim(matches[1].str()), trim(matches[2].str()));
            _modules.push_back(module);
            continue;
        }

        if (load_desc)
        {
            if (line[0] == '#')
            {
                p = Property();
                p._desc = trim(line.substr(1, line.length() - 1));
                load_desc = false;
                continue;
            }

            throw std::runtime_error("Expected a descriptive comment (#) before the property: " + line);
        }
        else
        {
            if (line.find('=') == std::string::npos)
                throw std::runtime_error("Malformed property line (missing '='): " + line);
            if (line[0] == '#')
            {
                p._disabled = true;
                line = line.substr(1, line.length() - 1);
            }

            std::smatch matches;
            if (!std::regex_search(line, matches, std::regex(R"(^\s*([^=]+?)\s*=\s*(.*?)\s*$)")))
                throw std::runtime_error("Parsing error in key-value pair: " + line);

            p._key = matches[1].str();
            p._value = matches[2].str();

            if (!module)
                throw std::runtime_error("Property found before defining any module: " + line);

            module->add(p);
            load_desc = true;
        }
    }
}

VlcRc::~VlcRc()
{
    for (auto i : _modules)
    {
        delete i;
    }
}

void VlcRc::add(Module *module)
{
    _modules.push_back(module);
}

void VlcRc::load(const std::string &filepath)
{
    std::ifstream input_file(filepath);
    if (!input_file.is_open())
    {
        throw std::runtime_error("Error: Could not open file '" + filepath + "' for reading");
    }

    try
    {
        load_modules(input_file);
        input_file.close();
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

void VlcRc::save(const std::string &filepath)
{
    std::ofstream out(filepath, std::ios::binary);

    for (auto &&module : _modules)
    {
        out << '[' << module->get_name() << "] # " << module->get_desc() << std::endl
            << std::endl;

        const auto props = module->properties();
        for (auto &&p : props)
        {
            out << "# " << p._desc << std::endl;
            out << (p._disabled ? "#" : "") << p._key << "=" << p._value << std::endl
                << std::endl;
        }
    }

    out.flush();
    out.close();
}

void VlcRc::remove(const std::string &name)
{
    for (size_t i = 0; i < _modules.size(); i++)
    {
        if (_modules[i]->get_name() == name)
        {
            delete _modules[i];
            _modules.erase(_modules.begin() + i);
            break;
        }
    }
}

Module *VlcRc::get(const std::string &name)
{
    for (auto i : _modules)
    {
        if (i->get_name() == name)
        {
            return i;
        }
    }

    return nullptr;
}

Module *VlcRc::get(int i)
{
    return _modules[i];
}

size_t VlcRc::size()
{
    return _modules.size();
}

int main(int argc, char const *argv[])
{
    if (argc <= 1)
    {
        std::cerr << "Number of invalid arguments" << std::endl;
        return 1;
    }

#ifdef _WIN32
    PWSTR pathTmp = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pathTmp);
    if (FAILED(hr))
    {
        std::cerr << "ERROR: " << hr << std::endl;
        return 1;
    }
	
	std::string roamingAppData = WstrToUtf8(pathTmp);
	CoTaskMemFree(pathTmp);

    std::string env = roamingAppData + "\\vlc\\vlcrc";
	
#elif defined(__linux__)
	std::string env;
	auto xdg_config = getenv("XDG_CONFIG_HOME");
	
	if (xdg_config)
		env = std::string(xdg_config) + "/vlc/vlcrc";
	else
	{
		const char* s_user = getenv("SUDO_USER");
		if (s_user)
			env = "/home/" + std::string(s_user) + "/.config/vlc/vlcrc";
		else
			env = std::string(getenv("HOME")) + "/.config/vlc/vlcrc";
	}
#else
	#error "Platform not supported"
#endif // _WIN32 || __linux__

    struct stat st;
    if (stat(env.c_str(), &st) != 0) // file exists?
    {
        std::cerr << "File not found: " << env;
        return 1;
    }

    try
    {
        if (std::string(argv[1]) == "--install")
        {
            VlcRc rc;
            rc.load(env);

            Module *m = rc.get("core");
            if (!m)
                throw std::runtime_error("Module [core] not found");

            Property *p = m->get("control");

            if (!p)
                throw std::runtime_error("key {control} not found");
            if (p->_value.find(PLUGIN_NAMESPACE) == std::string::npos)
            {
                const auto plugins = p->_value;
                bool is_empty = p->_value.empty();

                p->_value = std::string(PLUGIN_NAMESPACE);
                if (!is_empty)
                {
                    p->_value += ',';
                    p->_value += plugins;
                }
                else
                    p->_disabled = false;

                rc.save(env);
            }
        }
        else if (std::string(argv[1]) == "--uninstall")
        {
            VlcRc rc;
            rc.load(env);

            Module *module = rc.get("core");
            if (module)
            {
                Property *p = module->get("control");
                if (p && p->_value.find(PLUGIN_NAMESPACE) != std::string::npos)
                {
                    std::stringstream ss(p->_value);
                    std::string item;

                    std::string new_value = "";

                    while (std::getline(ss, item, ','))
                    {
                        std::string cleaned = trim(item);
                        if (!cleaned.empty() && cleaned != PLUGIN_NAMESPACE)
                        {
                            new_value += cleaned;
                            new_value += ",";
                        }
                    }

                    if (!new_value.empty())
                        new_value.pop_back();
                    else
                        p->_disabled = true;

                    p->_value = new_value;
                }
            }

            rc.remove(PLUGIN_NAMESPACE);
            rc.save(env);
        }
        else
        {
            std::cerr << "Usage: vlcrcedit [--install | --uninstall]" << std::endl;
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
