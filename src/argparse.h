#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <functional>
#include <list>
#include <map>
#include <string>

class ArgHandler {
public:
 ArgHandler(const std::string &name, const std::string &description,
     const std::string &help_group = "", const std::string &param_description = "");

 virtual ~ArgHandler() {}

 const std::string &Name() const { return _name; }

 const std::string &Description() const { return _description; }

 const std::string &ParamDescription() const { return _param_description; }

 const std::string &HelpGroup() const { return _help_group; }

 bool HandlingSucceeded() const { return _handling_succeeded; }

 /**
 * Parses the given list of arguments, looking for this particular handler's flag. Mutates the
 * argument list to remove parameters consumed by this handler. In the case of an error, user-
 * facing messages may be appended to out_messages. Sets _handling_succeeded to true if the arg
 * could not be handled correctly.
 *
 * \param arg_prefix The prefix characters used to identify a commandline param (e.g., "--").
 * \param args The list of arguments to be parsed.
 * \param out_messages A list of messages to be printed to the user.
 *
 * \return bool true if the parameter was found (regardless of whether it was parsed correctly).
 */
 bool ParseArglist(const std::string &arg_prefix, std::list<std::string> &args, std::list<std::string> &out_messages);

protected:
 typedef std::list<std::string> arglist_t;
 typedef arglist_t::const_iterator arglist_citerator_t;

 /**
 * Attempts to consume one or more arguments, returning the number consumed.
 *
 * \param arg_it Iterator to this argument handler's _name in the argument list.
 * \param arglist_end Iterator to the end of the argument list.
 * \param out_messages A list of messages to be printed to the user.
 *
 * \return int The number of arguments consumed by this handler.
 */
 virtual size_t HandleArg(arglist_citerator_t const &arg_it,
     arglist_citerator_t const &arglist_end, arglist_t &out_messages) = 0;

private:
 std::string _name;
 //! Description of the parameter to this flag, used to generate a help string.
 //! E.g., "--_name _param_description: _descriptions"
 std::string _param_description;
 std::string _description;
 std::string _help_group;  //!< Group under which this handler will be listed in the help string.

protected:
 bool _handling_succeeded;
};

/**
* An ArgHandler that sets the value of a bool if a flag is present.
*/
class FlagHandler : public ArgHandler {
public:
 /**
 * Constructs an ArgHandler that sets the value of flag to value_if_present.
 *
 * \param Name The parameter name to look for.
 * \param Description The help string for this argument.
 * \param flag The bool to be set.
 * \param value_if_present The value to set the flag bool to if the parameter is found.
 */
 FlagHandler(const std::string &name, const std::string &description, bool &flag,
     bool value_if_present = true, const std::string &help_group = "");

protected:
 virtual size_t HandleArg(arglist_citerator_t const &arg_it,
     arglist_citerator_t const &arglist_end,
     arglist_t &out_messages) override;

private:
 bool &_flag;
 bool _value_to_set;
};

/**
* An ArgHandler that calls a lambda if a flag is present.
*/
class LambdaFlagHandler : public ArgHandler {
public:
 typedef std::function<bool()> handler_t;

public:
 /**
 * Constructs an ArgHandler that calls the given lambda if present. The return value of the lambda
 * will be used to set HandlingSucceeded.
 *
 * \param Name The parameter name to look for.
 * \param Description The help string for this argument.
 * \param handler The lambda to be invoked if the param is found.
 */
 LambdaFlagHandler(const std::string &name, const std::string &description,
     const handler_t handler, const std::string &help_group = "");

protected:
 virtual size_t HandleArg(arglist_citerator_t const &arg_it,
     arglist_citerator_t const &arglist_end,
     arglist_t &out_messages) override;

private:
 handler_t _handler;
};

class LambdaArgHandler : public ArgHandler {
public:
 typedef std::function<bool(const std::string &param)> handler_t;

public:
 LambdaArgHandler(const std::string &name, const std::string &param_description,
     const std::string &description, handler_t handler,
     const std::string &help_group = "");

protected:
 virtual size_t HandleArg(const arglist_citerator_t &arg_it,
     const arglist_citerator_t &arglist_end,
     arglist_t &out_messages) override;

private:
 handler_t _handler;
};

class ArgParser {
public:
 ArgParser(const std::string &help_param = "help", const std::string &arg_prefix = "--");

 bool ShouldExit() const { return _should_exit; }

 const std::list<std::string> &UnusedArgs() const { return _unused_args; }

 /**
 *
 */
 bool ParseArgs(int argc, const char **argv);

 void PrintUserOutput() {
  for (auto line : _messages) {
   printf("%s\n", line.c_str());
  }
 }

 void AddHandler(std::shared_ptr<ArgHandler> handler) { AddHandler(0, handler); }

 void AddHandler(int pass, std::shared_ptr<ArgHandler> handler) {
  _handlers.emplace(std::make_pair(pass, handler));
  _help_map.emplace(std::make_pair(handler->HelpGroup(), handler));
 }

private:
 void BufferHelpMessage(const char *program_name);

private:
 typedef std::multimap<int, std::shared_ptr<ArgHandler> > handler_map_t;
 handler_map_t _handlers;

 typedef std::multimap<std::string, std::shared_ptr<ArgHandler> > help_map_t;
 help_map_t _help_map;

 std::list<std::string> _messages;  //!< Help/error strings that should be displayed to the user.
 std::string _help_param;  //!< The name used to display help strings and exit (default="help").
 std::string _arg_prefix;  //!< The string indicating a flag name (default="--").
 std::list<std::string> _unused_args;  //!< Arguments that were not parsed.
 bool _should_exit;  //!< Whether or not the program should exit due to error/help.
};


#endif  // #ifndef ARGPARSE_H
