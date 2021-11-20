// flipflip's c++ stuff: configuration file (imgui compatible)
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

#ifndef __FF_CONFFILE_HPP__
#define __FF_CONFFILE_HPP__

#include <string>
#include <map>

/* ****************************************************************************************************************** */

namespace Ff
{
    // Config file
    class ConfFile
    {
        public:
            ConfFile();

            void  Clear();

            bool  Load(const std::string &file, const std::string &section);
            bool  Save(const std::string &file, const std::string &section, const bool append = false);

            void  Set(const std::string &key, const std::string &val);
            void  Set(const std::string &key, const int32_t      val);
            void  Set(const std::string &key, const uint32_t     val, const bool hex = false);
            void  Set(const std::string &key, const double       val);
            void  Set(const std::string &key, const bool         val);

            bool  Get(const std::string &key, std::string &val);
            bool  Get(const std::string &key, int32_t     &val);
            bool  Get(const std::string &key, uint32_t    &val);
            bool  Get(const std::string &key, double      &val);
            bool  Get(const std::string &key, float       &val);
            bool  Get(const std::string &key, bool        &val);

            const std::map<std::string, std::string> &GetKeyValList();

            int GetSectionBeginLine();

        private:
            std::map<std::string, std::string> _keyVal;
            int         _lastLine;
            std::string _file;
            int         _sectionBeginLine;
    };
};

/* ****************************************************************************************************************** */

#endif // __FF_CONFFILE_HPP__
