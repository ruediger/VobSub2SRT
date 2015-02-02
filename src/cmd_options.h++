/*
 *  This file is part of vobsub2srt
 *
 *  Copyright (C) 2010-2015 Rüdiger Sonderfeld <ruediger@c-plusplus.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CMD_OPTIONS_HXX
#define CMD_OPTIONS_HXX

#include <string> // string_fwd in C++0x

/// Handle argc/argv
struct cmd_options {
  cmd_options(bool handle_help = true);
  ~cmd_options();

  cmd_options &add_option(char const *name, bool &val, char const *description, char short_name = '\0');
  cmd_options &add_option(char const *name, std::string &val, char const *description, char short_name = '\0');
  cmd_options &add_option(char const *name, int &val, char const *description, char short_name = '\0');

  cmd_options &add_unnamed(std::string &val, char const *help_name, char const *description);

  bool parse_cmd(int argc, char **argv) const;

  /// returns true if an error occurred and the program should exit
  bool should_exit() const {
    return exit;
  }
private:
  mutable bool exit;
  void help(char const *progname) const;
  bool handle_help;

  struct impl;
  impl *pimpl; // this should be a scoped_ptr

  // noncopyable
  cmd_options(cmd_options const&);
  cmd_options &operator=(cmd_options const&);
};

#endif
