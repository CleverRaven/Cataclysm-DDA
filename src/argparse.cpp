#include "argparse.h"

#include <algorithm>

ArgHandler::ArgHandler(const std::string &name, const std::string &description,
    const std::string &help_group, const std::string &param_description)
    : _name(name),
      _param_description(param_description),
      _description(description),
      _help_group(help_group),
      _handling_succeeded(true) {}

bool ArgHandler::ParseArglist(const std::string &arg_prefix, std::list<std::string> &args, std::list<std::string> &out_messages) {
  std::string arg_name = arg_prefix + _name;
  arglist_iterator_t it = args.begin();
  arglist_iterator_t it_end = args.end();
  for (; it != it_end; ++it) {
    if (*it == arg_name) {
      size_t args_to_pop = HandleArg(it, it_end, out_messages);
      arglist_iterator_t erase_end = it;
      std::advance(erase_end, args_to_pop);
      args.erase(it, erase_end);
      return true;
    }
  }
  return false;
}

FlagHandler::FlagHandler(const std::string &name, const std::string &description, bool &flag,
    bool value_if_present, const std::string &help_group)
    : ArgHandler(name, description, help_group), _flag(flag), _value_to_set(value_if_present) {}

size_t FlagHandler::HandleArg(const arglist_iterator_t &, const arglist_iterator_t &,
    arglist_t &) {
  _flag = _value_to_set;
  return 1;
}

LambdaFlagHandler::LambdaFlagHandler(const std::string &name, const std::string &description,
    const handler_t handler, const std::string &help_group)
    : ArgHandler(name, description, help_group), _handler(handler) {}

size_t LambdaFlagHandler::HandleArg(const arglist_iterator_t &,
    const arglist_iterator_t &,
    arglist_t &) {
  _handling_succeeded = _handler();
  return 1;
}

LambdaArgHandler::LambdaArgHandler(const std::string &name, const std::string &param_description,
    const std::string &description, handler_t handler,
    const std::string &help_group)
    : ArgHandler(name, description, help_group, param_description), _handler(handler) {}

size_t LambdaArgHandler::HandleArg(const arglist_iterator_t &arg_it,
    const arglist_iterator_t &arglist_end,
    arglist_t &out_messages) {
  arglist_iterator_t param = arg_it;
  if (++param == arglist_end) {
    std::string error_message = "Missing required argument to ";
    error_message += *arg_it;
    out_messages.push_back(error_message);
    _handling_succeeded = false;
    return 1;
  }
  _handling_succeeded = _handler(*param);
  return 2;
}

ArgParser::ArgParser(const std::string &help_param, const std::string &arg_prefix)
    : _help_param(help_param), _arg_prefix(arg_prefix), _should_exit(false) {}

bool ArgParser::ParseArgs(int argc, const char **argv) {
  if (argc == 0) return false;  // There must at least be a program name.
  if (argc < 2) return true;  // No params to parse.

  _unused_args.clear();
  for (int i = 1; i < argc; ++i) {
    _unused_args.push_back(argv[i]);
  }

  handler_map_t::const_iterator it = _handlers.cbegin();
  handler_map_t::const_iterator it_end = _handlers.cend();
  for (; it != it_end; ++it) {
    bool found = it->second->ParseArglist(_arg_prefix, _unused_args, _messages);
    if (found && !it->second->HandlingSucceeded()) {
      return false;
    }
  }

  // Search for the special --help param.
  std::string help_flag = _arg_prefix + _help_param;
  if (std::find(_unused_args.begin(), _unused_args.end(), help_flag) != _unused_args.end()) {
    BufferHelpMessage(argv[0]);
    _should_exit = true;
  }

  return true;
}

void ArgParser::BufferHelpMessage(const char *program_name) {
  _messages.push_back(std::string(program_name));
  _messages.push_back("\nCommand line parameters:");

  std::string param_indent = "\t";
  std::string param_description_separator = " ";
  std::string description_indent = "\t  ";

  help_map_t::const_iterator it = _help_map.cbegin();
  help_map_t::const_iterator it_end = _help_map.cend();
  std::string help_group;
  for (; it != it_end; ++it) {
    if (it->second->HelpGroup() != help_group) {
      help_group = it->second->HelpGroup();
      _messages.push_back(std::string("\n") + help_group);
    }
    std::string help_string = _arg_prefix;
    help_string += it->second->Name();

    const std::string &param_description = it->second->ParamDescription();
    if (!param_description.empty()) {
      help_string += param_description_separator + param_description;
    }
    _messages.push_back(param_indent + help_string);

    const std::string &description = it->second->Description();
    if (!description.empty()) {
      _messages.push_back(description_indent + description + "\n");
    }
  }
}
