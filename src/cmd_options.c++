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

#include "cmd_options.h++"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
using namespace std;

namespace {
struct option {
  enum arg_type { Bool, String, Int } type;
  union {
    bool *flag;
    std::string *str;
    int *i;
  } ref;
  char const *name;
  char const *description;
  char short_name;

  template<typename T>
  option(char const *name, arg_type type, T &r, char const *description, char short_name)
    : type(type), name(name), description(description), short_name(short_name)
  {
    switch(type) {
    case Bool:
      ref.flag = reinterpret_cast<bool*>(&r);
      break;
    case String:
      ref.str = reinterpret_cast<std::string*>(&r);
      break;
    case Int:
      ref.i = reinterpret_cast<int*>(&r);
      break;
    }
  }
};

struct unnamed {
  std::string *str;
  char const *name;
  char const *description;
  unnamed(std::string &s, char const *name, char const *description)
    : str(&s), name(name), description(description)
  { }
};

}

struct cmd_options::impl {
  std::vector<option> options;
  std::vector<unnamed> unnamed_args;
};

cmd_options::cmd_options(bool handle_help)
  : exit(false), handle_help(handle_help), pimpl(new impl)
{
  pimpl->options.reserve(10); // reserve a default amount of options here to save some reallocations
}

cmd_options::~cmd_options() {
  delete pimpl;
}

cmd_options &cmd_options::add_option(char const *name, bool &val, char const *description, char short_name) {
  pimpl->options.push_back(option(name, option::Bool, val, description, short_name));
  return *this;
}
cmd_options &cmd_options::add_option(char const *name, std::string &val, char const *description, char short_name) {
  pimpl->options.push_back(option(name, option::String, val, description, short_name));
  return *this;
}
cmd_options &cmd_options::add_option(char const *name, int &val, char const *description, char short_name) {
  pimpl->options.push_back(option(name, option::Int, val, description, short_name));
  return *this;
}

cmd_options &cmd_options::add_unnamed(std::string &val, char const *help_name, char const *description) {
  pimpl->unnamed_args.push_back(unnamed(val, help_name, description));
  return *this;
}

bool cmd_options::parse_cmd(int argc, char **argv) const {
  size_t current_unnamed = 0;
  bool parse_options = true; // set to false after --
  for(int i = 1; i < argc; ++i) {
    if(parse_options and argv[i][0] == '-') {
      unsigned offset = 1;
      if(argv[i][1] == '-') {
        if(argv[i][2] == '\0') {
          // "--" stops option parsing and handles only unnamed args (to allow e.g. files starting with -)
          parse_options = false;
        }
        offset = 2;
      }
      if(handle_help and (strcmp(argv[i]+offset, "help") == 0 or strcmp(argv[i]+offset, "h") == 0)) {
        help(argv[0]);
        continue;
      }
      bool known_option = false;
      for(std::vector<option>::const_iterator j = pimpl->options.begin(); j != pimpl->options.end(); ++j) {
        if(strcmp(argv[i]+offset, j->name) == 0 or
           (j->short_name != '\0' and offset == 1 and argv[i][1] == j->short_name and argv[i][2] == '\0')) {
          known_option = true;
          if(j->type == option::Bool) {
            *j->ref.flag = true;
            break;
          }

          if(i+1 >= argc or argv[i+1][0] == '-') { // Check if next argv is an option or argument
            cerr << "option " << argv[i] << " is missing an argument\n";
            exit = true;
            return false;
          }
          ++i;

          if(j->type == option::String) {
            *j->ref.str = argv[i];
          }
          else if(j->type == option::Int) {
            char *endptr;
            *j->ref.i = strtol(argv[i], &endptr, 10);
            if(*endptr != '\0') {
              cerr << "option " << argv[i] << " expects a number as argument but got '" << argv[i] << "'\n";
              exit = true;
              return false;
            }
          }
          break;
        }
      }
      if(not known_option) {
        cerr << "ERROR: unknown option '" << argv[i] << "'\n";
        help(argv[0]);
      }
    }
    else if(pimpl->unnamed_args.size() > current_unnamed) {
      *pimpl->unnamed_args[current_unnamed].str = argv[i];
      ++current_unnamed;
    }
    else {
      help(argv[0]);
    }
  }
  return not exit;
}

void cmd_options::help(char const *progname) const {
  cerr << "usage: " << progname;
  for(std::vector<option>::const_iterator i = pimpl->options.begin(); i != pimpl->options.end(); ++i) {
    cerr << " --" << i->name;
  }
  if(handle_help) {
    cerr << " --help";
  }
  for(std::vector<unnamed>::const_iterator i = pimpl->unnamed_args.begin(); i != pimpl->unnamed_args.end(); ++i) {
    cerr << " <" << i->name << '>';
  }
  cerr << "\n\n";
  for(std::vector<option>::const_iterator i = pimpl->options.begin(); i != pimpl->options.end(); ++i) {
    cerr << "\t--" << i->name;
    if(i->type != option::Bool) {
      cerr << " <arg>";
    }
    if(i->short_name != '\0') {
      cerr << " (or -" << i->short_name << ')';
    }
    cerr << "\t... " << i->description << '\n';
  }
  if(handle_help) {
    cerr << "\t--help (or -h)\t... show help information\n";
  }
  for(std::vector<unnamed>::const_iterator i = pimpl->unnamed_args.begin(); i != pimpl->unnamed_args.end(); ++i) {
    cerr << "\t<" << i->name << ">\t... " << i->description << '\n';
  }
  exit = true;
}
