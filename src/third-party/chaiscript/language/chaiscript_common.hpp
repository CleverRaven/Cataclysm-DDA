// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_COMMON_HPP_
#define CHAISCRIPT_COMMON_HPP_

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../chaiscript_defines.hpp"
#include "../dispatchkit/boxed_value.hpp"
#include "../dispatchkit/dispatchkit.hpp"
#include "../dispatchkit/proxy_functions.hpp"
#include "../dispatchkit/type_info.hpp"

namespace chaiscript {
struct AST_Node;
}  // namespace chaiscript

namespace chaiscript
{
  struct Name_Validator {
    static bool is_reserved_word(const std::string &name)
    {
      static const std::set<std::string> m_reserved_words 
        = {"def", "fun", "while", "for", "if", "else", "&&", "||", ",", "auto", 
          "return", "break", "true", "false", "class", "attr", "var", "global", "GLOBAL", "_",
          "__LINE__", "__FILE__", "__FUNC__", "__CLASS__"};
      return m_reserved_words.count(name) > 0;
    }

    static bool valid_object_name(const std::string &name)
    {
      return name.find("::") == std::string::npos && !is_reserved_word(name);
    }

    static void validate_object_name(const std::string &name)
    {
      if (is_reserved_word(name)) {
        throw exception::reserved_word_error(name);
      }

      if (name.find("::") != std::string::npos) {
        throw exception::illegal_name_error(name);
      }
    }
  };

  /// Signature of module entry point that all binary loadable modules must implement.
  typedef ModulePtr (*Create_Module_Func)();


  /// Types of AST nodes available to the parser and eval
  enum class AST_Node_Type { Id, Fun_Call, Unused_Return_Fun_Call, Arg_List, Equation, Var_Decl, Assign_Decl,
    Array_Call, Dot_Access,
    Lambda, Block, Scopeless_Block, Def, While, If, For, Ranged_For, Inline_Array, Inline_Map, Return, File, Prefix, Break, Continue, Map_Pair, Value_Range,
    Inline_Range, Try, Catch, Finally, Method, Attr_Decl,  
    Logical_And, Logical_Or, Reference, Switch, Case, Default, Noop, Class, Binary, Arg, Global_Decl, Constant, Compiled
  };

  enum class Operator_Precidence { Ternary_Cond, Logical_Or, 
    Logical_And, Bitwise_Or, Bitwise_Xor, Bitwise_And, 
    Equality, Comparison, Shift, Addition, Multiplication, Prefix };

  namespace
  {
    /// Helper lookup to get the name of each node type
    inline const char *ast_node_type_to_string(AST_Node_Type ast_node_type) {
      static const char * const ast_node_types[] = { "Id", "Fun_Call", "Unused_Return_Fun_Call", "Arg_List", "Equation", "Var_Decl", "Assign_Decl",
                                    "Array_Call", "Dot_Access", 
                                    "Lambda", "Block", "Scopeless_Block", "Def", "While", "If", "For", "Ranged_For", "Inline_Array", "Inline_Map", "Return", "File", "Prefix", "Break", "Continue", "Map_Pair", "Value_Range",
                                    "Inline_Range", "Try", "Catch", "Finally", "Method", "Attr_Decl",
                                    "Logical_And", "Logical_Or", "Reference", "Switch", "Case", "Default", "Noop", "Class", "Binary", "Arg", "Global_Decl", "Constant", "Compiled"};

      return ast_node_types[static_cast<int>(ast_node_type)];
    }
  }

  /// \brief Convenience type for file positions
  struct File_Position {
    int line;
    int column;

    File_Position(int t_file_line, int t_file_column)
      : line(t_file_line), column(t_file_column) { }

    File_Position() : line(0), column(0) { }
  };

  struct Parse_Location {
    Parse_Location(std::string t_fname="", const int t_start_line=0, const int t_start_col=0,
        const int t_end_line=0, const int t_end_col=0)
      : start(t_start_line, t_start_col), 
        end(t_end_line, t_end_col),
        filename(std::make_shared<std::string>(std::move(t_fname)))
    {
    }

    Parse_Location(std::shared_ptr<std::string> t_fname, const int t_start_line=0, const int t_start_col=0,
        const int t_end_line=0, const int t_end_col=0)
      : start(t_start_line, t_start_col), 
        end(t_end_line, t_end_col),
        filename(std::move(t_fname))
    {
    }



    File_Position start;
    File_Position end;
    std::shared_ptr<std::string> filename;
  };


  /// \brief Typedef for pointers to AST_Node objects. Used in building of the AST_Node tree
  typedef std::unique_ptr<AST_Node> AST_NodePtr;
  typedef std::unique_ptr<const AST_Node> AST_NodePtr_Const;

  struct AST_Node_Trace;


  /// \brief Classes which may be thrown during error cases when ChaiScript is executing.
  namespace exception
  {
    /// \brief Thrown if an error occurs while attempting to load a binary module
    struct load_module_error : std::runtime_error
    {
      explicit load_module_error(const std::string &t_reason) noexcept
        : std::runtime_error(t_reason)
      {
      }

      load_module_error(const std::string &t_name, const std::vector<load_module_error> &t_errors)
        : std::runtime_error(format_error(t_name, t_errors))
      {
      }

      load_module_error(const load_module_error &) = default;
      ~load_module_error() noexcept override = default;

      static std::string format_error(const std::string &t_name, const std::vector<load_module_error> &t_errors)
      {
        std::stringstream ss;
        ss << "Error loading module '" << t_name << "'\n"
           << "  The following locations were searched:\n";

        for (const auto &err : t_errors) {
          ss << "    " << err.what() << "\n";
        }

        return ss.str();
      }
    };


    /// Errors generated during parsing or evaluation
    struct eval_error : std::runtime_error {
      std::string reason;
      File_Position start_position;
      std::string filename;
      std::string detail;
      std::vector<AST_Node_Trace> call_stack;

      eval_error(const std::string &t_why, const File_Position &t_where, const std::string &t_fname,
          const std::vector<Boxed_Value> &t_parameters, const std::vector<chaiscript::Const_Proxy_Function> &t_functions,
          bool t_dot_notation,
          const chaiscript::detail::Dispatch_Engine &t_ss) noexcept :
        std::runtime_error(format(t_why, t_where, t_fname, t_parameters, t_dot_notation, t_ss)),
        reason(t_why), start_position(t_where), filename(t_fname), detail(format_detail(t_functions, t_dot_notation, t_ss)) 
      {}

      eval_error(const std::string &t_why, 
           const std::vector<Boxed_Value> &t_parameters, const std::vector<chaiscript::Const_Proxy_Function> &t_functions,
           bool t_dot_notation,
           const chaiscript::detail::Dispatch_Engine &t_ss) noexcept :
        std::runtime_error(format(t_why, t_parameters, t_dot_notation, t_ss)),
        reason(t_why), detail(format_detail(t_functions, t_dot_notation, t_ss))
      {}


      eval_error(const std::string &t_why, const File_Position &t_where, const std::string &t_fname) noexcept :
        std::runtime_error(format(t_why, t_where, t_fname)),
        reason(t_why), start_position(t_where), filename(t_fname)
      {}

      explicit eval_error(const std::string &t_why) noexcept
        : std::runtime_error("Error: \"" + t_why + "\" "),
        reason(t_why) 
      {}

      eval_error(const eval_error &) = default;

      std::string pretty_print() const
      {
        std::ostringstream ss;

        ss << what();
        if (!call_stack.empty()) {
          ss << "during evaluation at (" << fname(call_stack[0]) << " " << startpos(call_stack[0]) << ")\n";
          ss << '\n' << detail << '\n';
          ss << "  " << fname(call_stack[0]) << " (" << startpos(call_stack[0]) << ") '" << pretty(call_stack[0]) << "'";
          for (size_t j = 1; j < call_stack.size(); ++j) {
            if (id(call_stack[j]) != chaiscript::AST_Node_Type::Block
                && id(call_stack[j]) != chaiscript::AST_Node_Type::File)
            {
              ss << '\n';
              ss << "  from " << fname(call_stack[j]) << " (" << startpos(call_stack[j]) << ") '" << pretty(call_stack[j]) << "'";
            }
          }
        }
        ss << '\n';
        return ss.str();
      }

      ~eval_error() noexcept override = default;

    private:

      template<typename T>
        static AST_Node_Type id(const T& t)
        {
          return t.identifier;
        }

      template<typename T>
        static std::string pretty(const T& t)
        {
          return t.pretty_print();
        }

      template<typename T>
        static const std::string &fname(const T& t)
        {
          return t.filename();
        }

      template<typename T>
        static std::string startpos(const T& t)
        {
          std::ostringstream oss;
          oss << t.start().line << ", " << t.start().column;
          return oss.str();
        }

      static std::string format_why(const std::string &t_why)
      {
        return "Error: \"" + t_why + "\"";
      }

      static std::string format_types(const Const_Proxy_Function &t_func,
          bool t_dot_notation,
          const chaiscript::detail::Dispatch_Engine &t_ss)
      {
        assert(t_func);
        int arity = t_func->get_arity();
        std::vector<Type_Info> types = t_func->get_param_types();

        std::string retval;
        if (arity == -1)
        {
          retval = "(...)";
          if (t_dot_notation)
          {
            retval = "(Object)." + retval;
          }
        } else if (types.size() <= 1) {
          retval = "()";
        } else {
          std::stringstream ss;
          ss << "(";

          std::string paramstr;

          for (size_t index = 1;
               index != types.size();
               ++index)
          {
            paramstr += (types[index].is_const()?"const ":"");
            paramstr += t_ss.get_type_name(types[index]);

            if (index == 1 && t_dot_notation)
            {
              paramstr += ").(";
              if (types.size() == 2)
              {
                paramstr += ", ";
              }
            } else {
              paramstr += ", ";
            }
          }

          ss << paramstr.substr(0, paramstr.size() - 2);

          ss << ")";
          retval = ss.str();
        }


        std::shared_ptr<const dispatch::Dynamic_Proxy_Function> dynfun 
          = std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(t_func);

        if (dynfun && dynfun->has_parse_tree())
        {
          Proxy_Function f = dynfun->get_guard();

          if (f)
          {
            auto dynfunguard = std::dynamic_pointer_cast<const dispatch::Dynamic_Proxy_Function>(f);
            if (dynfunguard && dynfunguard->has_parse_tree())
            {
              retval += " : " + format_guard(dynfunguard->get_parse_tree());
            }
          }

          retval += "\n          Defined at " + format_location(dynfun->get_parse_tree());
        }

        return retval;
      }

      template<typename T>
        static std::string format_guard(const T &t)
        {
          return t.pretty_print();
        }

      template<typename T>
        static std::string format_location(const T &t)
        {
          std::ostringstream oss;
          oss << "(" << t.filename() << " " << t.start().line << ", " << t.start().column << ")"; 
          return oss.str();
        }

      static std::string format_detail(const std::vector<chaiscript::Const_Proxy_Function> &t_functions,
          bool t_dot_notation,
          const chaiscript::detail::Dispatch_Engine &t_ss)
      {
        std::stringstream ss;
        if (t_functions.size() == 1)
        {
          assert(t_functions[0]);
          ss << "  Expected: " << format_types(t_functions[0], t_dot_notation, t_ss) << '\n';
        } else {
          ss << "  " << t_functions.size() << " overloads available:\n";

          for (const auto & t_function : t_functions)
          {
            ss << "      " << format_types((t_function), t_dot_notation, t_ss) << '\n';
          }

        }

        return ss.str();

      }

      static std::string format_parameters(const std::vector<Boxed_Value> &t_parameters,
          bool t_dot_notation,
          const chaiscript::detail::Dispatch_Engine &t_ss)
      {
        std::stringstream ss;
        ss << "(";

        if (!t_parameters.empty())
        {
          std::string paramstr;

          for (auto itr = t_parameters.begin();
               itr != t_parameters.end();
               ++itr)
          {
            paramstr += (itr->is_const()?"const ":"");
            paramstr += t_ss.type_name(*itr);

            if (itr == t_parameters.begin() && t_dot_notation)
            {
              paramstr += ").(";
              if (t_parameters.size() == 1)
              {
                paramstr += ", ";
              }
            } else {
              paramstr += ", ";
            }
          }

          ss << paramstr.substr(0, paramstr.size() - 2);
        }
        ss << ")";

        return ss.str();
      }

      static std::string format_filename(const std::string &t_fname)
      {
        std::stringstream ss;

        if (t_fname != "__EVAL__")
        {
          ss << "in '" << t_fname << "' ";
        } else {
          ss << "during evaluation ";
        }

        return ss.str();
      }

      static std::string format_location(const File_Position &t_where)
      {
        std::stringstream ss;
        ss << "at (" << t_where.line << ", " << t_where.column << ")";
        return ss.str();
      }

      static std::string format(const std::string &t_why, const File_Position &t_where, const std::string &t_fname,
          const std::vector<Boxed_Value> &t_parameters, bool t_dot_notation, const chaiscript::detail::Dispatch_Engine &t_ss)
      {
        std::stringstream ss;

        ss << format_why(t_why);
        ss << " ";

        ss << "With parameters: " << format_parameters(t_parameters, t_dot_notation, t_ss);
        ss << " ";

        ss << format_filename(t_fname);
        ss << " ";

        ss << format_location(t_where);

        return ss.str();
      }

      static std::string format(const std::string &t_why, 
          const std::vector<Boxed_Value> &t_parameters, 
          bool t_dot_notation,
          const chaiscript::detail::Dispatch_Engine &t_ss)
      {
        std::stringstream ss;

        ss << format_why(t_why);
        ss << " ";

        ss << "With parameters: " << format_parameters(t_parameters, t_dot_notation, t_ss);
        ss << " ";

        return ss.str();
      }

      static std::string format(const std::string &t_why, const File_Position &t_where, const std::string &t_fname)
      {
        std::stringstream ss;

        ss << format_why(t_why);
        ss << " ";

        ss << format_filename(t_fname);
        ss << " ";

        ss << format_location(t_where);

        return ss.str();
      }
    };


    /// Errors generated when loading a file
    struct file_not_found_error : std::runtime_error {
      explicit file_not_found_error(const std::string &t_filename) noexcept
        : std::runtime_error("File Not Found: " + t_filename),
          filename(t_filename)
      { }

      file_not_found_error(const file_not_found_error &) = default;
      ~file_not_found_error() noexcept override = default;

      std::string filename;
    };

  }

 
  /// \brief Struct that doubles as both a parser ast_node and an AST node.
  struct AST_Node {
    public:
      const AST_Node_Type identifier;
      const std::string text;
      Parse_Location location;

      const std::string &filename() const {
        return *location.filename;
      }

      const File_Position &start() const {
        return location.start;
      }

      const File_Position &end() const {
        return location.end;
      }

      std::string pretty_print() const
      {
        std::ostringstream oss;

        oss << text;

        for (auto & elem : get_children()) {
          oss << elem.get().pretty_print() << ' ';
        }

        return oss.str();
      }

      virtual std::vector<std::reference_wrapper<AST_Node>> get_children() const = 0;
      virtual Boxed_Value eval(const chaiscript::detail::Dispatch_State &t_e) const = 0;


      /// Prints the contents of an AST node, including its children, recursively
      std::string to_string(const std::string &t_prepend = "") const {
        std::ostringstream oss;

        oss << t_prepend << "(" << ast_node_type_to_string(this->identifier) << ") "
            << this->text << " : " << this->location.start.line << ", " << this->location.start.column << '\n';

        for (auto & elem : get_children()) {
          oss << elem.get().to_string(t_prepend + "  ");
        }
        return oss.str();
      }


      static bool get_bool_condition(const Boxed_Value &t_bv, const chaiscript::detail::Dispatch_State &t_ss) {
        try {
          return t_ss->boxed_cast<bool>(t_bv);
        }
        catch (const exception::bad_boxed_cast &) {
          throw exception::eval_error("Condition not boolean");
        }
      }


      virtual ~AST_Node() = default;
      AST_Node(AST_Node &&) = default;
      AST_Node &operator=(AST_Node &&) = default;
      AST_Node(const AST_Node &) = delete;
      AST_Node& operator=(const AST_Node &) = delete;


    protected:
      AST_Node(std::string t_ast_node_text, AST_Node_Type t_id, Parse_Location t_loc)
        : identifier(t_id), text(std::move(t_ast_node_text)),
          location(std::move(t_loc))
      {
      }


  };

  struct AST_Node_Trace
  {
    const AST_Node_Type identifier;
    const std::string text;
    Parse_Location location;

    const std::string &filename() const {
      return *location.filename;
    }

    const File_Position &start() const {
      return location.start;
    }

    const File_Position &end() const {
      return location.end;
    }

    std::string pretty_print() const
    {
      std::ostringstream oss;

      oss << text;

      for (const auto & elem : children) {
        oss << elem.pretty_print() << ' ';
      }

      return oss.str();
    }

    std::vector<AST_Node_Trace> get_children(const AST_Node &node)
    {
      const auto node_children = node.get_children();
      return std::vector<AST_Node_Trace>(node_children.begin(), node_children.end());
    }

    AST_Node_Trace(const AST_Node &node)
      : identifier(node.identifier), text(node.text),
      location(node.location), children(get_children(node))
    {
    }


    std::vector<AST_Node_Trace> children;

  };

  namespace parser {
    class ChaiScript_Parser_Base
    {
      public:
        virtual AST_NodePtr parse(const std::string &t_input, const std::string &t_fname) = 0;
        virtual void debug_print(const AST_Node &t, std::string prepend = "") const = 0;
        virtual void *get_tracer_ptr() = 0;
        virtual ~ChaiScript_Parser_Base() = default;
        ChaiScript_Parser_Base() = default;
        ChaiScript_Parser_Base(ChaiScript_Parser_Base &&) = default;
        ChaiScript_Parser_Base &operator=(ChaiScript_Parser_Base &&) = delete;
        ChaiScript_Parser_Base &operator=(const ChaiScript_Parser_Base &&) = delete;

        template<typename T>
        T &get_tracer()
        {
          // to do type check this somehow?
          return *static_cast<T*>(get_tracer_ptr());
        }

      protected:
        ChaiScript_Parser_Base(const ChaiScript_Parser_Base &) = default;
    };
  }

  namespace eval
  {
    namespace detail
    {
      /// Special type for returned values
      struct Return_Value {
        Boxed_Value retval;
      };


      /// Special type indicating a call to 'break'
      struct Break_Loop {
        Break_Loop() = default;
      };


      /// Special type indicating a call to 'continue'
      struct Continue_Loop {
        Continue_Loop() = default;
      };


      /// Creates a new scope then pops it on destruction
      struct Scope_Push_Pop
      {
        Scope_Push_Pop(Scope_Push_Pop &&) = default;
        Scope_Push_Pop& operator=(Scope_Push_Pop &&) = default;
        Scope_Push_Pop(const Scope_Push_Pop &) = delete;
        Scope_Push_Pop& operator=(const Scope_Push_Pop &) = delete;

        explicit Scope_Push_Pop(const chaiscript::detail::Dispatch_State &t_ds)
          : m_ds(t_ds)
        {
          m_ds->new_scope(m_ds.stack_holder());
        }

        ~Scope_Push_Pop()
        {
          m_ds->pop_scope(m_ds.stack_holder());
        }


        private:
          const chaiscript::detail::Dispatch_State &m_ds;
      };

      /// Creates a new function call and pops it on destruction
      struct Function_Push_Pop
      {
        Function_Push_Pop(Function_Push_Pop &&) = default;
        Function_Push_Pop& operator=(Function_Push_Pop &&) = default;
        Function_Push_Pop(const Function_Push_Pop &) = delete;
        Function_Push_Pop& operator=(const Function_Push_Pop &) = delete;

        explicit Function_Push_Pop(const chaiscript::detail::Dispatch_State &t_ds)
          : m_ds(t_ds)
        {
          m_ds->new_function_call(m_ds.stack_holder(), m_ds.conversion_saves());
        }

        ~Function_Push_Pop()
        {
          m_ds->pop_function_call(m_ds.stack_holder(), m_ds.conversion_saves());
        }

        void save_params(const std::vector<Boxed_Value> &t_params)
        {
          m_ds->save_function_params(t_params);
        }

        void save_params(std::initializer_list<Boxed_Value> t_params)
        {
          m_ds->save_function_params(t_params);
        }


        private:
          const chaiscript::detail::Dispatch_State &m_ds;
      };

      /// Creates a new scope then pops it on destruction
      struct Stack_Push_Pop
      {
        Stack_Push_Pop(Stack_Push_Pop &&) = default;
        Stack_Push_Pop& operator=(Stack_Push_Pop &&) = default;
        Stack_Push_Pop(const Stack_Push_Pop &) = delete;
        Stack_Push_Pop& operator=(const Stack_Push_Pop &) = delete;

        explicit Stack_Push_Pop(const chaiscript::detail::Dispatch_State &t_ds)
          : m_ds(t_ds)
        {
          m_ds->new_stack(m_ds.stack_holder());
        }

        ~Stack_Push_Pop()
        {
          m_ds->pop_stack(m_ds.stack_holder());
        }


        private:
          const chaiscript::detail::Dispatch_State &m_ds;
      };
    }
  }
}

#endif /* _CHAISCRIPT_COMMON_HPP */

