// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com


#ifndef CHAISCRIPT_PARSER_HPP_
#define CHAISCRIPT_PARSER_HPP_

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstring>




#include "../dispatchkit/boxed_value.hpp"
#include "chaiscript_common.hpp"
#include "chaiscript_optimizer.hpp"
#include "chaiscript_tracer.hpp"
#include "../utility/fnv1a.hpp"
#include "../utility/static_string.hpp"

#if defined(CHAISCRIPT_UTF16_UTF32)
#include <locale>
#include <codecvt>
#endif

#if defined(CHAISCRIPT_MSVC) && defined(max) && defined(min)
#define CHAISCRIPT_PUSHED_MIN_MAX
#pragma push_macro("max") // Why Microsoft? why? This is worse than bad
#undef max
#pragma push_macro("min")
#undef min
#endif


namespace chaiscript
{
  /// \brief Classes and functions used during the parsing process.
  namespace parser
  {
    /// \brief Classes and functions internal to the parsing process. Not supported for the end user.
    namespace detail 
    {
      enum Alphabet
      {   symbol_alphabet = 0
        ,   keyword_alphabet
          ,   int_alphabet
          ,   float_alphabet
          ,   x_alphabet
          ,   hex_alphabet
          ,   b_alphabet
          ,   bin_alphabet
          ,   id_alphabet
          ,   white_alphabet
          ,   int_suffix_alphabet
          ,   float_suffix_alphabet
          ,   max_alphabet
          ,   lengthof_alphabet = 256
      };

      // Generic for u16, u32 and wchar
      template<typename string_type>
      struct Char_Parser_Helper
      {
        // common for all implementations
        static std::string u8str_from_ll(long long val)
        {
          typedef std::string::value_type char_type;

          char_type c[2];
          c[1] = char_type(val);
          c[0] = char_type(val >> 8);

          if (c[0] == 0)
          {
            return std::string(1, c[1]); // size, character
          }

          return std::string(c, 2); // char buffer, size
        }

        static string_type str_from_ll(long long val)
        {
          typedef typename string_type::value_type target_char_type;
#if defined (CHAISCRIPT_UTF16_UTF32)
          // prepare converter
          std::wstring_convert<std::codecvt_utf8<target_char_type>, target_char_type> converter;
          // convert
          return converter.from_bytes(u8str_from_ll(val));
#else
          // no conversion available, just put value as character
          return string_type(1, target_char_type(val)); // size, character
#endif
        }
      };

      // Specialization for char AKA UTF-8
      template<>
      struct Char_Parser_Helper<std::string>
      {
        static std::string str_from_ll(long long val)
        {
          // little SFINAE trick to avoid base class
          return Char_Parser_Helper<std::true_type>::u8str_from_ll(val);
        }
      };
    }


    template<typename Tracer, typename Optimizer, std::size_t Parse_Depth=512>
    class ChaiScript_Parser final : public ChaiScript_Parser_Base {
      void *get_tracer_ptr() override {
        return &m_tracer;
      }

      std::size_t m_current_parse_depth = 0;

      struct Depth_Counter
      {
        static const auto max_depth = Parse_Depth;
        Depth_Counter(ChaiScript_Parser *t_parser) : parser(t_parser)
        {
          ++parser->m_current_parse_depth;
          if (parser->m_current_parse_depth > max_depth) {
            throw exception::eval_error("Maximum parse depth exceeded", 
                File_Position(parser->m_position.line, parser->m_position.col), *(parser->m_filename));
          }
        }

        ~Depth_Counter() noexcept
        {
          --parser->m_current_parse_depth;
        }

        ChaiScript_Parser *parser;
      };

      static std::array<std::array<bool, detail::lengthof_alphabet>, detail::max_alphabet> build_alphabet()
      {
        std::array<std::array<bool, detail::lengthof_alphabet>, detail::max_alphabet> alphabet;

        for (auto &alpha : alphabet) {
          alpha.fill(false);
        }

        alphabet[detail::symbol_alphabet][static_cast<size_t>('?')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('+')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('-')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('*')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('/')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('|')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('&')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('^')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('=')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('.')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('<')]=true;
        alphabet[detail::symbol_alphabet][static_cast<size_t>('>')]=true;

        for ( size_t c = 'a' ; c <= 'z' ; ++c ) { alphabet[detail::keyword_alphabet][c]=true; }
        for ( size_t c = 'A' ; c <= 'Z' ; ++c ) { alphabet[detail::keyword_alphabet][c]=true; }
        for ( size_t c = '0' ; c <= '9' ; ++c ) { alphabet[detail::keyword_alphabet][c]=true; }
        alphabet[detail::keyword_alphabet][static_cast<size_t>('_')]=true;

        for ( size_t c = '0' ; c <= '9' ; ++c ) { alphabet[detail::int_alphabet][c]=true; }
        for ( size_t c = '0' ; c <= '9' ; ++c ) { alphabet[detail::float_alphabet][c]=true; }
        alphabet[detail::float_alphabet][static_cast<size_t>('.')]=true;

        for ( size_t c = '0' ; c <= '9' ; ++c ) { alphabet[detail::hex_alphabet][c]=true; }
        for ( size_t c = 'a' ; c <= 'f' ; ++c ) { alphabet[detail::hex_alphabet][c]=true; }
        for ( size_t c = 'A' ; c <= 'F' ; ++c ) { alphabet[detail::hex_alphabet][c]=true; }

        alphabet[detail::x_alphabet][static_cast<size_t>('x')]=true;
        alphabet[detail::x_alphabet][static_cast<size_t>('X')]=true;

        for ( size_t c = '0' ; c <= '1' ; ++c ) { alphabet[detail::bin_alphabet][c]=true; }
        alphabet[detail::b_alphabet][static_cast<size_t>('b')]=true;
        alphabet[detail::b_alphabet][static_cast<size_t>('B')]=true;

        for ( size_t c = 'a' ; c <= 'z' ; ++c ) { alphabet[detail::id_alphabet][c]=true; }
        for ( size_t c = 'A' ; c <= 'Z' ; ++c ) { alphabet[detail::id_alphabet][c]=true; }
        alphabet[detail::id_alphabet][static_cast<size_t>('_')] = true;

        alphabet[detail::white_alphabet][static_cast<size_t>(' ')]=true;
        alphabet[detail::white_alphabet][static_cast<size_t>('\t')]=true;

        alphabet[detail::int_suffix_alphabet][static_cast<size_t>('l')] = true;
        alphabet[detail::int_suffix_alphabet][static_cast<size_t>('L')] = true;
        alphabet[detail::int_suffix_alphabet][static_cast<size_t>('u')] = true;
        alphabet[detail::int_suffix_alphabet][static_cast<size_t>('U')] = true;

        alphabet[detail::float_suffix_alphabet][static_cast<size_t>('l')] = true;
        alphabet[detail::float_suffix_alphabet][static_cast<size_t>('L')] = true;
        alphabet[detail::float_suffix_alphabet][static_cast<size_t>('f')] = true;
        alphabet[detail::float_suffix_alphabet][static_cast<size_t>('F')] = true;

        return alphabet;
      }

      static const std::array<std::array<bool, detail::lengthof_alphabet>, detail::max_alphabet> &create_alphabet()
      {
        static const auto alpha = build_alphabet();
        return alpha;
      }


      static const std::vector<std::vector<utility::Static_String>> &create_operator_matches() {
        static const std::vector<std::vector<utility::Static_String>> operator_matches {
          {"?"},
          {"||"},
          {"&&"},
          {"|"},
          {"^"},
          {"&"},
          {"==", "!="},
          {"<", "<=", ">", ">="},
          {"<<", ">>"},
          //We share precedence here but then separate them later
          {"+", "-"},
          {"*", "/", "%"},
          {"++", "--", "-", "+", "!", "~"}
        };

        return operator_matches;
      }


      static const std::array<Operator_Precidence, 12> &create_operators() {
        static const std::array<Operator_Precidence, 12> operators = { {
          Operator_Precidence::Ternary_Cond,
          Operator_Precidence::Logical_Or,
          Operator_Precidence::Logical_And,
          Operator_Precidence::Bitwise_Or,
          Operator_Precidence::Bitwise_Xor,
          Operator_Precidence::Bitwise_And,
          Operator_Precidence::Equality,
          Operator_Precidence::Comparison,
          Operator_Precidence::Shift,
          Operator_Precidence::Addition,
          Operator_Precidence::Multiplication,
          Operator_Precidence::Prefix
        } };
        return operators;
      }

      static const utility::Static_String &multiline_comment_end()
      {
        static const utility::Static_String s("*/");
        return s;
      }

      static const utility::Static_String &multiline_comment_begin()
      {
        static const utility::Static_String s("/*");
        return s;
      }

      static const utility::Static_String &singleline_comment()
      {
        static const utility::Static_String s("//");
        return s;
      }

      static const utility::Static_String &annotation()
      {
        static const utility::Static_String s("#");
        return s;
      }

      static const utility::Static_String &cr_lf()
      {
        static const utility::Static_String s("\r\n");
        return s;
      }

      const std::array<std::array<bool, detail::lengthof_alphabet>, detail::max_alphabet> &m_alphabet = create_alphabet();
      const std::vector<std::vector<utility::Static_String>> &m_operator_matches = create_operator_matches();
      const std::array<Operator_Precidence, 12> &m_operators = create_operators();

      std::shared_ptr<std::string> m_filename;
      std::vector<eval::AST_Node_Impl_Ptr<Tracer>> m_match_stack;


      struct Position
      {
        Position() = default;

        Position(std::string::const_iterator t_pos, std::string::const_iterator t_end)
          : line(1), col(1), m_pos(t_pos), m_end(t_end), m_last_col(1)
        {
        }

        static std::string str(const Position &t_begin, const Position &t_end) {
          return std::string(t_begin.m_pos, t_end.m_pos);
        }

        Position &operator++() {
          if (m_pos != m_end) {
            if (*m_pos == '\n') {
              ++line;
              m_last_col = std::exchange(col, 1);
            } else {
              ++col;
            }

            ++m_pos;
          }
          return *this;
        }

        Position &operator--() {
          --m_pos;
          if (*m_pos == '\n') {
            --line;
            col = m_last_col;
          } else {
            --col;
          }
          return *this;
        }

        Position &operator+=(size_t t_distance) {
          *this = (*this) + t_distance;
          return *this;
        }

        Position operator+(size_t t_distance) const {
          Position ret(*this);
          for (size_t i = 0; i < t_distance; ++i) {
            ++ret;
          }
          return ret;
        }

        Position &operator-=(size_t t_distance) {
          *this = (*this) - t_distance;
          return *this;
        }

        Position operator-(size_t t_distance) const {
          Position ret(*this);
          for (size_t i = 0; i < t_distance; ++i) {
            --ret;
          }
          return ret;
        }

        bool operator==(const Position &t_rhs) const {
          return m_pos == t_rhs.m_pos;
        }

        bool operator!=(const Position &t_rhs) const {
          return m_pos != t_rhs.m_pos;
        }

        bool has_more() const {
          return m_pos != m_end;
        }

        size_t remaining() const {
          return static_cast<size_t>(std::distance(m_pos, m_end));
        }

        const char& operator*() const {
          if (m_pos == m_end) {
            static const char ktmp ='\0';
            return ktmp;
          } else {
            return *m_pos;
          }
        }

        int line = -1;
        int col = -1;

        private:
          std::string::const_iterator m_pos;
          std::string::const_iterator m_end;
          int m_last_col = -1;
      };

      Position m_position;

      Tracer m_tracer;
      Optimizer m_optimizer;

      void validate_object_name(const std::string &name) const
      {
        if (!Name_Validator::valid_object_name(name)) {
          throw exception::eval_error("Invalid Object Name: " + name, File_Position(m_position.line, m_position.col), *m_filename);
        }
      }

      public:
      explicit ChaiScript_Parser(Tracer tracer = Tracer(), Optimizer optimizer=Optimizer())
        : m_tracer(std::move(tracer)),
          m_optimizer(std::move(optimizer))
      {
        m_match_stack.reserve(2);
      }

      Tracer &get_tracer() 
      {
        return m_tracer;
      }

      Optimizer &get_optimizer()
      {
        return m_optimizer;
      }

      ChaiScript_Parser(const ChaiScript_Parser &) = delete;
      ChaiScript_Parser &operator=(const ChaiScript_Parser &) = delete;
      ChaiScript_Parser(ChaiScript_Parser &&) = default;
      ChaiScript_Parser &operator=(ChaiScript_Parser &&) = delete;

      /// test a char in an m_alphabet
      bool char_in_alphabet(char c, detail::Alphabet a) const { return m_alphabet[a][static_cast<uint8_t>(c)]; }

      /// Prints the parsed ast_nodes as a tree
      void debug_print(const AST_Node &t, std::string prepend = "") const override {
        std::cout << prepend << "(" << ast_node_type_to_string(t.identifier) << ") " << t.text << " : " << t.start().line << ", " << t.start().column << '\n';
        for (const auto &node : t.get_children()) {
          debug_print(node.get(), prepend + "  ");
        }
      }


      /// Helper function that collects ast_nodes from a starting position to the top of the stack into a new AST node
      template<typename NodeType>
      void build_match(size_t t_match_start, std::string t_text = "") {
        bool is_deep = false;

        Parse_Location filepos = [&]()->Parse_Location{ 
          //so we want to take everything to the right of this and make them children
          if (t_match_start != m_match_stack.size()) {
            is_deep = true;
            return Parse_Location(
                m_filename,
                m_match_stack[t_match_start]->location.start.line,
                m_match_stack[t_match_start]->location.start.column,
                m_position.line,
                m_position.col 
              );
          } else {
            return Parse_Location(
                m_filename,
                m_position.line,
                m_position.col,
                m_position.line,
                m_position.col
              );
          }
        }();

        std::vector<eval::AST_Node_Impl_Ptr<Tracer>> new_children;

        if (is_deep) {
          new_children.assign(std::make_move_iterator(m_match_stack.begin() + static_cast<int>(t_match_start)), 
                              std::make_move_iterator(m_match_stack.end()));
          m_match_stack.erase(m_match_stack.begin() + static_cast<int>(t_match_start), m_match_stack.end());
        }

        /// \todo fix the fact that a successful match that captured no ast_nodes doesn't have any real start position
        m_match_stack.push_back(
            m_optimizer.optimize(
              chaiscript::make_unique<chaiscript::eval::AST_Node_Impl<Tracer>, NodeType>(
                std::move(t_text),
                std::move(filepos),
                std::move(new_children)))
            );
      }


      /// Reads a symbol group from input if it matches the parameter, without skipping initial whitespace
      inline auto Symbol_(const utility::Static_String &sym)
      {
        const auto len = sym.size();
        if (m_position.remaining() >= len) {
          const char *file_pos = &(*m_position);
          for (size_t pos = 0; pos < len; ++pos)
          {
            if (sym.c_str()[pos] != file_pos[pos]) { return false; }
          }
          m_position += len;
          return true;
        }
        return false;
      }

      /// Skips any multi-line or single-line comment
      bool SkipComment() {
        if (Symbol_(multiline_comment_begin())) {
          while (m_position.has_more()) {
            if (Symbol_(multiline_comment_end())) {
              break;
            } else if (!Eol_()) {
              ++m_position;
            }
          }
          return true;
        } else if (Symbol_(singleline_comment())) {
          while (m_position.has_more()) {
            if (Symbol_(cr_lf())) {
              m_position -= 2;
              break;
            } else if (Char_('\n')) {
              --m_position;
              break;
            } else {
              ++m_position;
            }
          }
          return true;
        } else if (Symbol_(annotation())) {
          while (m_position.has_more()) {
            if (Symbol_(cr_lf())) {
              m_position -= 2;
              break;
            } else if (Char_('\n')) {
              --m_position;
              break;
            } else {
              ++m_position;
            }
          }
          return true;
        }
        return false;
      }


      /// Skips ChaiScript whitespace, which means space and tab, but not cr/lf
      /// jespada: Modified SkipWS to skip optionally CR ('\n') and/or LF+CR ("\r\n")
      /// AlekMosingiewicz: Added exception when illegal character detected
      bool SkipWS(bool skip_cr=false) {
        bool retval = false;

        while (m_position.has_more()) {
          if(static_cast<unsigned char>(*m_position) > 0x7e) {
            throw exception::eval_error("Illegal character", File_Position(m_position.line, m_position.col), *m_filename);
          }
          auto end_line = (*m_position != 0) && ((*m_position == '\n') || (*m_position == '\r' && *(m_position+1) == '\n'));

          if ( char_in_alphabet(*m_position,detail::white_alphabet) || (skip_cr && end_line)) {

            if(end_line) {
              if(*m_position == '\r') {
                // discards lf
                ++m_position;
              }
            }

            ++m_position;

            retval = true;
          }
          else if (SkipComment()) {
            retval = true;
          } else {
            break;
          }
        }
        return retval;
      }

      /// Reads the optional exponent (scientific notation) and suffix for a Float
      bool read_exponent_and_suffix() {
        // Support a form of scientific notation: 1e-5, 35.5E+8, 0.01e19
        if (m_position.has_more() && (std::tolower(*m_position) == 'e')) {
          ++m_position;
          if (m_position.has_more() && ((*m_position == '-') || (*m_position == '+'))) {
            ++m_position;
          }
          auto exponent_pos = m_position;
          while (m_position.has_more() && char_in_alphabet(*m_position,detail::int_alphabet) ) {
            ++m_position;
          }
          if (m_position == exponent_pos) {
            // Require at least one digit after the exponent
            return false;
          }
        }

        // Parse optional float suffix
        while (m_position.has_more() && char_in_alphabet(*m_position, detail::float_suffix_alphabet))
        {
          ++m_position;
        }

        return true;
      }


      /// Reads a floating point value from input, without skipping initial whitespace
      bool Float_() {
        if (m_position.has_more() && char_in_alphabet(*m_position,detail::float_alphabet) ) {
          while (m_position.has_more() && char_in_alphabet(*m_position,detail::int_alphabet) ) {
            ++m_position;
          }

          if (m_position.has_more() && (std::tolower(*m_position) == 'e')) {
            // The exponent is valid even without any decimal in the Float (1e8, 3e-15)
            return read_exponent_and_suffix();
          }
          else if (m_position.has_more() && (*m_position == '.')) {
            ++m_position;
            if (m_position.has_more() && char_in_alphabet(*m_position,detail::int_alphabet)) {
              while (m_position.has_more() && char_in_alphabet(*m_position,detail::int_alphabet) ) {
                ++m_position;
              }

              // After any decimal digits, support an optional exponent (3.7e3)
              return read_exponent_and_suffix();
            } else {
              --m_position;
            }
          }
        }
        return false;
      }

      /// Reads a hex value from input, without skipping initial whitespace
      bool Hex_() {
        if (m_position.has_more() && (*m_position == '0')) {
          ++m_position;

          if (m_position.has_more() && char_in_alphabet(*m_position, detail::x_alphabet) ) {
            ++m_position;
            if (m_position.has_more() && char_in_alphabet(*m_position, detail::hex_alphabet)) {
              while (m_position.has_more() && char_in_alphabet(*m_position, detail::hex_alphabet) ) {
                ++m_position;
              }
              while (m_position.has_more() && char_in_alphabet(*m_position, detail::int_suffix_alphabet))
              {
                ++m_position;
              }

              return true;
            }
            else {
              --m_position;
            }
          }
          else {
            --m_position;
          }
        }

        return false;
      }

      /// Reads an integer suffix
      void IntSuffix_() {
        while (m_position.has_more() && char_in_alphabet(*m_position, detail::int_suffix_alphabet))
        {
          ++m_position;
        }
      }

      /// Reads a binary value from input, without skipping initial whitespace
      bool Binary_() {
        if (m_position.has_more() && (*m_position == '0')) {
          ++m_position;

          if (m_position.has_more() && char_in_alphabet(*m_position, detail::b_alphabet) ) {
            ++m_position;
            if (m_position.has_more() && char_in_alphabet(*m_position, detail::bin_alphabet) ) {
              while (m_position.has_more() && char_in_alphabet(*m_position, detail::bin_alphabet) ) {
                ++m_position;
              }
              return true;
            } else {
              --m_position;
            }
          } else {
            --m_position;
          }
        }

        return false;
      }

      /// Parses a floating point value and returns a Boxed_Value representation of it
      static Boxed_Value buildFloat(const std::string &t_val)
      {
        bool float_ = false;
        bool long_ = false;

        auto i = t_val.size();

        for (; i > 0; --i)
        {
          char val = t_val[i-1];

          if (val == 'f' || val == 'F')
          {
            float_ = true;
          } else if (val == 'l' || val == 'L') {
            long_ = true;
          } else {
            break;
          }
        }

        if (float_)
        {
          return const_var(parse_num<float>(t_val.substr(0,i)));
        } else if (long_) {
          return const_var(parse_num<long double>(t_val.substr(0,i)));
        } else {
          return const_var(parse_num<double>(t_val.substr(0,i)));
        }
      }



      static Boxed_Value buildInt(const int base, const std::string &t_val, const bool prefixed)
      {
        bool unsigned_ = false;
        bool long_ = false;
        bool longlong_ = false;

        auto i = t_val.size();

        for (; i > 0; --i)
        {
          const char val = t_val[i-1];

          if (val == 'u' || val == 'U')
          {
            unsigned_ = true;
          } else if (val == 'l' || val == 'L') {
            if (long_)
            {
              longlong_ = true;
            }

            long_ = true;
          } else {
            break;
          }
        }

        const auto val = prefixed?std::string(t_val.begin()+2,t_val.end()):t_val;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

#ifdef CHAISCRIPT_CLANG
#pragma GCC diagnostic ignored "-Wtautological-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#endif

        try {
          auto u = std::stoll(val,nullptr,base);


          if (!unsigned_ && !long_ && u >= std::numeric_limits<int>::min() && u <= std::numeric_limits<int>::max()) {
            return const_var(static_cast<int>(u));
          } else if ((unsigned_ || base != 10) && !long_ && u >= std::numeric_limits<unsigned int>::min() && u <= std::numeric_limits<unsigned int>::max()) {
            return const_var(static_cast<unsigned int>(u));
          } else if (!unsigned_ && !longlong_ && u >= std::numeric_limits<long>::min() && u <= std::numeric_limits<long>::max()) {
            return const_var(static_cast<long>(u));
          } else if ((unsigned_ || base != 10) && !longlong_

                     && u >= std::numeric_limits<unsigned long>::min()
                     && u <= std::numeric_limits<unsigned long>::max()) {
            return const_var(static_cast<unsigned long>(u));
          } else if (!unsigned_ && u >= std::numeric_limits<long long>::min() && u <= std::numeric_limits<long long>::max()) {
            return const_var(static_cast<long long>(u));
          } else {
            return const_var(static_cast<unsigned long long>(u));
          }

        } catch (const std::out_of_range &) {
          // too big to be signed
          try {
            auto u = std::stoull(val,nullptr,base);

            if (!longlong_ && u >= std::numeric_limits<unsigned long>::min() && u <= std::numeric_limits<unsigned long>::max()) {
              return const_var(static_cast<unsigned long>(u));
            } else {
              return const_var(static_cast<unsigned long long>(u));
            }
          } catch (const std::out_of_range &) {
            // it's just simply too big
            return const_var(std::numeric_limits<long long>::max());
          }
        }

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

      }

      template<typename T, typename ... Param>
      std::unique_ptr<eval::AST_Node_Impl<Tracer>> make_node(std::string t_match, const int t_prev_line, const int t_prev_col, Param && ...param)
      {
        return chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, T>(std::move(t_match), Parse_Location(m_filename, t_prev_line, t_prev_col, m_position.line, m_position.col), std::forward<Param>(param)...);
      }

      /// Reads a number from the input, detecting if it's an integer or floating point
      bool Num() {
        SkipWS();

        const auto start = m_position;
        if (m_position.has_more() && char_in_alphabet(*m_position, detail::float_alphabet) ) {
          try {
            if (Hex_()) {
              auto match = Position::str(start, m_position);
              auto bv = buildInt(16, match, true);
              m_match_stack.emplace_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(match), start.line, start.col, std::move(bv)));
              return true;
            }

            if (Binary_()) {
              auto match = Position::str(start, m_position);
              auto bv = buildInt(2, match, true);
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(match), start.line, start.col, std::move(bv)));
              return true;
            }
            if (Float_()) {
              auto match = Position::str(start, m_position);
              auto bv = buildFloat(match);
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(match), start.line, start.col, std::move(bv)));
              return true;
            }
            else {
              IntSuffix_();
              auto match = Position::str(start, m_position);
              if (!match.empty() && (match[0] == '0')) {
                auto bv = buildInt(8, match, false);
                m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(match), start.line, start.col, std::move(bv)));
              }
              else if (!match.empty()) {
                auto bv = buildInt(10, match, false);
                m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(match), start.line, start.col, std::move(bv)));
              } else {
                return false;
              }
              return true;
            }
          } catch (const std::invalid_argument &) {
            // error parsing number passed in to buildFloat/buildInt
            return false;
          }
        }
        else {
          return false;
        }
      }

      /// Reads an identifier from input which conforms to C's identifier naming conventions, without skipping initial whitespace
      bool Id_() {
        if (m_position.has_more() && char_in_alphabet(*m_position, detail::id_alphabet)) {
          while (m_position.has_more() && char_in_alphabet(*m_position, detail::keyword_alphabet) ) {
            ++m_position;
          }

          return true;
        } else if (m_position.has_more() && (*m_position == '`')) {
          ++m_position;
          const auto start = m_position;

          while (m_position.has_more() && (*m_position != '`')) {
            if (Eol()) {
              throw exception::eval_error("Carriage return in identifier literal", File_Position(m_position.line, m_position.col), *m_filename);
            }
            else {
              ++m_position;
            }
          }

          if (start == m_position) {
            throw exception::eval_error("Missing contents of identifier literal", File_Position(m_position.line, m_position.col), *m_filename);
          }
          else if (!m_position.has_more()) {
            throw exception::eval_error("Incomplete identifier literal", File_Position(m_position.line, m_position.col), *m_filename);
          }

          ++m_position;

          return true;
        }
        return false;
      }

      /// Reads (and potentially captures) an identifier from input
      bool Id(const bool validate) {
        SkipWS();

        const auto start = m_position;
        if (Id_()) {

          auto text = Position::str(start, m_position);
          const auto text_hash = utility::fnv1a_32(text.c_str());

          if (validate) {
            validate_object_name(text);
          }

#ifdef CHAISCRIPT_MSVC
#pragma warning(push)
#pragma warning(disable : 4307)
#endif

          switch (text_hash) {
            case utility::fnv1a_32("true"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col, const_var(true)));
            } break;
            case utility::fnv1a_32("false"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col, const_var(false)));
            } break;
            case utility::fnv1a_32("Infinity"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(std::numeric_limits<double>::infinity())));
            } break;
            case utility::fnv1a_32("NaN"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(std::numeric_limits<double>::quiet_NaN())));
            } break;
            case utility::fnv1a_32("__LINE__"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(start.line)));
            } break;
            case utility::fnv1a_32("__FILE__"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(m_filename)));
            } break;
            case utility::fnv1a_32("__FUNC__"): {
              std::string fun_name = "NOT_IN_FUNCTION";
              for (size_t idx = m_match_stack.empty() ? 0 : m_match_stack.size() - 1; idx > 0; --idx)
              {
                if (m_match_stack[idx-1]->identifier == AST_Node_Type::Id
                    && m_match_stack[idx-0]->identifier == AST_Node_Type::Arg_List) {
                  fun_name = m_match_stack[idx-1]->text;
                }
              }

              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(fun_name)));
            } break;
            case utility::fnv1a_32("__CLASS__"): {
              std::string fun_name = "NOT_IN_CLASS";
              for (size_t idx = m_match_stack.empty() ? 0 : m_match_stack.size() - 1; idx > 1; --idx)
              {
                if (m_match_stack[idx-2]->identifier == AST_Node_Type::Id
                    && m_match_stack[idx-1]->identifier == AST_Node_Type::Id
                    && m_match_stack[idx-0]->identifier == AST_Node_Type::Arg_List) {
                  fun_name = m_match_stack[idx-2]->text;
                }
              }

              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                const_var(fun_name)));
            } break;
            case utility::fnv1a_32("_"): {
              m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(std::move(text), start.line, start.col,
                Boxed_Value(std::make_shared<dispatch::Placeholder_Object>())));
            } break;
            default: {
                std::string val = std::move(text);
              if (*start == '`') {
                // 'escaped' literal, like an operator name
                val = Position::str(start+1, m_position-1);
              }
              m_match_stack.push_back(make_node<eval::Id_AST_Node<Tracer>>(val, start.line, start.col));
            } break;
          }

#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif


          return true;
        } else {
          return false;
        }
      }

      /// Reads an argument from input
      bool Arg(const bool t_type_allowed = true) {
        const auto prev_stack_top = m_match_stack.size();
        SkipWS();

        if (!Id(true)) {
          return false;
        }

        SkipWS();

        if (t_type_allowed) {
          Id(true);
        }

        build_match<eval::Arg_AST_Node<Tracer>>(prev_stack_top);

        return true;
      }



      /// Reads a quoted string from input, without skipping initial whitespace
      bool Quoted_String_() {
        if (m_position.has_more() && (*m_position == '\"')) {
          char prev_char = *m_position;
          ++m_position;

          int in_interpolation = 0;
          bool in_quote = false;

          while (m_position.has_more() && ((*m_position != '\"') || (in_interpolation > 0) || (prev_char == '\\'))) {

            if (!Eol_()) {
              if (prev_char == '$' && *m_position == '{') {
                ++in_interpolation;
              } else if (prev_char != '\\' && *m_position == '"') {
                in_quote = !in_quote;
              } else if (*m_position == '}' && !in_quote) {
                --in_interpolation;
              }

              if (prev_char == '\\') {
                prev_char = 0;
              } else {
                prev_char = *m_position;
              }
              ++m_position;
            }
          }

          if (m_position.has_more()) {
            ++m_position;
          } else {
            throw exception::eval_error("Unclosed quoted string", File_Position(m_position.line, m_position.col), *m_filename);
          }

          return true;
        }
        return false;
      }

      template<typename string_type>
      struct Char_Parser
      {
        string_type &match;
        typedef typename string_type::value_type char_type;
        bool is_escaped = false;
        bool is_interpolated = false;
        bool saw_interpolation_marker = false;
        bool is_octal = false;
        bool is_hex = false;
        std::size_t unicode_size = 0;
        const bool interpolation_allowed;

        string_type octal_matches;
        string_type hex_matches;

        Char_Parser(string_type &t_match, const bool t_interpolation_allowed)
          : match(t_match),
            interpolation_allowed(t_interpolation_allowed)
        {
        }

        Char_Parser &operator=(const Char_Parser &) = delete;

        ~Char_Parser(){
          try {
            if (is_octal) {
              process_octal();
            }

            if (is_hex) {
              process_hex();
            }

            if (unicode_size > 0) {
              process_unicode();
            }
          } catch (const std::invalid_argument &) {
          } catch (const exception::eval_error &) {
            // Something happened with parsing, we'll catch it later?
          }
        }

        void process_hex()
        {
          if (!hex_matches.empty()) {
            auto val = stoll(hex_matches, nullptr, 16);
            match.push_back(char_type(val));
          }
          hex_matches.clear();
          is_escaped = false;
          is_hex = false;
        }


        void process_octal()
        {
          if (!octal_matches.empty()) {
            auto val = stoll(octal_matches, nullptr, 8);
            match.push_back(char_type(val));
          }
          octal_matches.clear();
          is_escaped = false;
          is_octal = false;
        }


        void process_unicode()
        {
          const auto ch = static_cast<uint32_t>(std::stoi(hex_matches, nullptr, 16));
          const auto match_size = hex_matches.size();
          hex_matches.clear();
          is_escaped = false;
          const auto u_size = unicode_size;
          unicode_size = 0;

          char buf[4];
          if (u_size != match_size) {
            throw exception::eval_error("Incomplete unicode escape sequence");
          }
          if (u_size == 4 && ch >= 0xD800 && ch <= 0xDFFF) {
            throw exception::eval_error("Invalid 16 bit universal character");
          }


          if (ch < 0x80) {
            match += static_cast<char>(ch);
          } else if (ch < 0x800) {
            buf[0] = static_cast<char>(0xC0 | (ch >> 6));
            buf[1] = static_cast<char>(0x80 | (ch & 0x3F));
            match.append(buf, 2);
          } else if (ch < 0x10000) {
            buf[0] = static_cast<char>(0xE0 |  (ch >> 12));
            buf[1] = static_cast<char>(0x80 | ((ch >>  6) & 0x3F));
            buf[2] = static_cast<char>(0x80 |  (ch        & 0x3F));
            match.append(buf, 3);
          } else if (ch < 0x200000) {
            buf[0] = static_cast<char>(0xF0 |  (ch >> 18));
            buf[1] = static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
            buf[2] = static_cast<char>(0x80 | ((ch >>  6) & 0x3F));
            buf[3] = static_cast<char>(0x80 |  (ch        & 0x3F));
            match.append(buf, 4);
          } else {
            // this must be an invalid escape sequence?
            throw exception::eval_error("Invalid 32 bit universal character");
          }
        }

        void parse(const char_type t_char, const int line, const int col, const std::string &filename) {
          const bool is_octal_char = t_char >= '0' && t_char <= '7';

          const bool is_hex_char  = (t_char >= '0' && t_char <= '9')
                                 || (t_char >= 'a' && t_char <= 'f')
                                 || (t_char >= 'A' && t_char <= 'F');

          if (is_octal) {
            if (is_octal_char) {
              octal_matches.push_back(t_char);

              if (octal_matches.size() == 3) {
                process_octal();
              }
              return;
            } else {
              process_octal();
            }
          } else if (is_hex) {
            if (is_hex_char) {
              hex_matches.push_back(t_char);

              if (hex_matches.size() == 2*sizeof(char_type)) {
                // This rule differs from the C/C++ standard, but ChaiScript
                // does not offer the same workaround options, and having
                // hexadecimal sequences longer than can fit into the char
                // type is undefined behavior anyway.
                process_hex();
              }
              return;
            } else {
              process_hex();
            }
          } else if (unicode_size > 0) {
            if (is_hex_char) {
              hex_matches.push_back(t_char);

              if(hex_matches.size() == unicode_size) {
                // Format is specified to be 'slash'uABCD
                // on collecting from A to D do parsing
                process_unicode();
              }
              return;
            } else {
              // Not a unicode anymore, try parsing any way
              // May be someone used 'slash'uAA only
              process_unicode();
            }
          }

          if (t_char == '\\') {
            if (is_escaped) {
              match.push_back('\\');
              is_escaped = false;
            } else {
              is_escaped = true;
            }
          } else {
            if (is_escaped) {
              if (is_octal_char) {
                is_octal = true;
                octal_matches.push_back(t_char);
              } else if (t_char == 'x') {
                is_hex = true;
              } else if (t_char == 'u') {
                unicode_size = 4;
              } else if (t_char == 'U') {
                unicode_size = 8;
              } else {
                switch (t_char) {
                  case ('\'') : match.push_back('\''); break;
                  case ('\"') : match.push_back('\"'); break;
                  case ('?') : match.push_back('?'); break;
                  case ('a') : match.push_back('\a'); break;
                  case ('b') : match.push_back('\b'); break;
                  case ('f') : match.push_back('\f'); break;
                  case ('n') : match.push_back('\n'); break;
                  case ('r') : match.push_back('\r'); break;
                  case ('t') : match.push_back('\t'); break;
                  case ('v') : match.push_back('\v'); break;
                  case ('$') : match.push_back('$'); break;
                  default: throw exception::eval_error("Unknown escaped sequence in string", File_Position(line, col), filename);
                }
                is_escaped = false;
              }
            } else if (interpolation_allowed && t_char == '$') {
              saw_interpolation_marker = true;
            } else {
              match.push_back(t_char);
            }
          }
        }

      };


      /// Reads (and potentially captures) a quoted string from input.  Translates escaped sequences.
      bool Quoted_String() {
        Depth_Counter dc{this};
        SkipWS();

        const auto start = m_position;

        if (Quoted_String_()) {
          std::string match;
          const auto prev_stack_top = m_match_stack.size();

          bool is_interpolated = [&]()->bool {
            Char_Parser<std::string> cparser(match, true);


            auto s = start + 1, end = m_position - 1;

            while (s != end) {
              if (cparser.saw_interpolation_marker) {
                if (*s == '{') {
                  //We've found an interpolation point

                  m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(match, start.line, start.col, const_var(match)));

                  if (cparser.is_interpolated) {
                    //If we've seen previous interpolation, add on instead of making a new one
                    build_match<eval::Binary_Operator_AST_Node<Tracer>>(prev_stack_top, "+");
                  }

                  //We've finished with the part of the string up to this point, so clear it
                  match.clear();

                  std::string eval_match;

                  ++s;
                  while ((s != end) && (*s != '}')) {
                    eval_match.push_back(*s);
                    ++s;
                  }

                  if (*s == '}') {
                    cparser.is_interpolated = true;
                    ++s;

                    const auto tostr_stack_top = m_match_stack.size();

                    m_match_stack.push_back(make_node<eval::Id_AST_Node<Tracer>>("to_string", start.line, start.col));

                    const auto ev_stack_top = m_match_stack.size();

                    try {
                      m_match_stack.push_back(parse_instr_eval(eval_match));
                    } catch (const exception::eval_error &e) {
                      throw exception::eval_error(e.what(), File_Position(start.line, start.col), *m_filename);
                    }

                    build_match<eval::Arg_List_AST_Node<Tracer>>(ev_stack_top);
                    build_match<eval::Fun_Call_AST_Node<Tracer>>(tostr_stack_top);
                    build_match<eval::Binary_Operator_AST_Node<Tracer>>(prev_stack_top, "+");
                  } else {
                    throw exception::eval_error("Unclosed in-string eval", File_Position(start.line, start.col), *m_filename);
                  }
                } else {
                  match.push_back('$');
                }
                cparser.saw_interpolation_marker = false;
              } else {
                cparser.parse(*s, start.line, start.col, *m_filename);

                ++s;
              }
            }

            return cparser.is_interpolated;
          }();

          m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(match, start.line, start.col, const_var(match)));

          if (is_interpolated) {
            build_match<eval::Binary_Operator_AST_Node<Tracer>>(prev_stack_top, "+");
          }

          return true;
        } else {
          return false;
        }
      }

      /// Reads a character group from input, without skipping initial whitespace
      bool Single_Quoted_String_() {
        bool retval = false;
        if (m_position.has_more() && (*m_position == '\'')) {
          retval = true;
          char prev_char = *m_position;
          ++m_position;

          while (m_position.has_more() && ((*m_position != '\'') || (prev_char == '\\'))) {
            if (!Eol_()) {
              if (prev_char == '\\') {
                prev_char = 0;
              } else {
                prev_char = *m_position;
              }
              ++m_position;
            }
          }

          if (m_position.has_more()) {
            ++m_position;
          } else {
            throw exception::eval_error("Unclosed single-quoted string", File_Position(m_position.line, m_position.col), *m_filename);
          }
        }
        return retval;
      }

      /// Reads (and potentially captures) a char group from input.  Translates escaped sequences.
      bool Single_Quoted_String() {
        Depth_Counter dc{this};
        SkipWS();

        const auto start = m_position;
        if (Single_Quoted_String_()) {
          std::string match;

          {
            // scope for cparser destructor
            Char_Parser<std::string> cparser(match, false);

            for (auto s = start + 1, end = m_position - 1; s != end; ++s) {
              cparser.parse(*s, start.line, start.col, *m_filename);
            }
          }

          if (match.size() != 1) {
            throw exception::eval_error("Single-quoted strings must be 1 character long", File_Position(m_position.line, m_position.col), *m_filename);
          }

          m_match_stack.push_back(make_node<eval::Constant_AST_Node<Tracer>>(match, start.line, start.col, const_var(char(match.at(0)))));
          return true;
        }
        else {
          return false;
        }
      }

      /// Reads a char from input if it matches the parameter, without skipping initial whitespace
      bool Char_(const char c) {
        if (m_position.has_more() && (*m_position == c)) {
          ++m_position;
          return true;
        } else {
          return false;
        }
      }

      /// Reads (and potentially captures) a char from input if it matches the parameter
      bool Char(const char t_c) {
        Depth_Counter dc{this};
        SkipWS();
        return Char_(t_c);
      }

      /// Reads a string from input if it matches the parameter, without skipping initial whitespace
      bool Keyword_(const utility::Static_String &t_s) {
        const auto len = t_s.size();
        if (m_position.remaining() >= len) {
          auto tmp = m_position;
          for (size_t i = 0; tmp.has_more() && i < len; ++i) {
            if (*tmp != t_s.c_str()[i]) {
              return false;
            }
            ++tmp;
          }
          m_position = tmp;
          return true;
        }

        return false;
      }

      /// Reads (and potentially captures) a string from input if it matches the parameter
      bool Keyword(const utility::Static_String &t_s) {
        Depth_Counter dc{this};
        SkipWS();
        const auto start = m_position;
        bool retval = Keyword_(t_s);
        // ignore substring matches
        if ( retval && m_position.has_more() && char_in_alphabet(*m_position, detail::keyword_alphabet) ) {
          m_position = start;
          retval = false;
        }

        return retval;
      }

      bool is_operator(const std::string &t_s) const {
        return std::any_of(m_operator_matches.begin(), m_operator_matches.end(),
            [t_s](const std::vector<utility::Static_String> &opers) {
              return std::any_of(opers.begin(), opers.end(), 
                [t_s](const utility::Static_String &s) {
                  return t_s == s.c_str();
                });
            });
      }

      /// Reads (and potentially captures) a symbol group from input if it matches the parameter
      bool Symbol(const utility::Static_String &t_s, const bool t_disallow_prevention=false) {
        Depth_Counter dc{this};
        SkipWS();
        const auto start = m_position;
        bool retval = Symbol_(t_s);

        // ignore substring matches
        if (retval && m_position.has_more() && (t_disallow_prevention == false) && char_in_alphabet(*m_position,detail::symbol_alphabet)) {
          if (*m_position != '=' && is_operator(Position::str(start, m_position)) && !is_operator(Position::str(start, m_position+1))) {
            // don't throw this away, it's a good match and the next is not
          } else {
            m_position = start;
            retval = false;
          }
        }

        return retval;
      }

      /// Reads an end-of-line group from input, without skipping initial whitespace
      bool Eol_(const bool t_eos = false) {
        bool retval = false;

        if (m_position.has_more() && (Symbol_(cr_lf()) || Char_('\n'))) {
          retval = true;
          //++m_position.line;
          m_position.col = 1;
        } else if (m_position.has_more() && !t_eos && Char_(';')) {
          retval = true;
        }

        return retval;
      }

      /// Reads until the end of the current statement
      bool Eos() {
        Depth_Counter dc{this};
        SkipWS();

        return Eol_(true);
      }

      /// Reads (and potentially captures) an end-of-line group from input
      bool Eol() {
        Depth_Counter dc{this};
        SkipWS();

        return Eol_();
      }

      /// Reads a comma-separated list of values from input. Id's only, no types allowed
      bool Id_Arg_List() {
        Depth_Counter dc{this};
        SkipWS(true);
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Arg(false)) {
          retval = true;
          while (Eol()) {}

          while (Char(',')) {
            while (Eol()) {}
            if (!Arg(false)) {
              throw exception::eval_error("Unexpected value in parameter list", File_Position(m_position.line, m_position.col), *m_filename);
            }
          } 
        }
        build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);

        SkipWS(true);

        return retval;
      }

      /// Reads a comma-separated list of values from input, for function declarations
      bool Decl_Arg_List() {
        Depth_Counter dc{this};
        SkipWS(true);
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Arg()) {
          retval = true;
          while (Eol()) {}

          while (Char(',')) {
            while (Eol()) {}
            if (!Arg()) {
              throw exception::eval_error("Unexpected value in parameter list", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }
        }
        build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);

        SkipWS(true);

        return retval;
      }


      /// Reads a comma-separated list of values from input
      bool Arg_List() {
        Depth_Counter dc{this};
        SkipWS(true);
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Equation()) {
          retval = true;
          while (Eol()) {}
          while (Char(',')) {
            while (Eol()) {}
            if (!Equation()) {
              throw exception::eval_error("Unexpected value in parameter list", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }
        }

        build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);

        SkipWS(true);

        return retval;
      }

      /// Reads possible special container values, including ranges and map_pairs
      bool Container_Arg_List() {
        Depth_Counter dc{this};
        bool retval = false;
        SkipWS(true);

        const auto prev_stack_top = m_match_stack.size();

        if (Value_Range()) {
          retval = true;
          build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);
        } else if (Map_Pair()) {
          retval = true;
          while (Eol()) {}
          while (Char(',')) {
            while (Eol()) {}
            if (!Map_Pair()) {
              throw exception::eval_error("Unexpected value in container", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }
          build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);
        } else if (Operator()) {
          retval = true;
          while (Eol()) {}
          while (Char(',')) {
            while (Eol()) {}
            if (!Operator()) {
              throw exception::eval_error("Unexpected value in container", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }
          build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);
        }

        SkipWS(true);

        return retval;
      }

      /// Reads a lambda (anonymous function) from input
      bool Lambda() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("fun")) {
          retval = true;

          if (Char('[')) {
            Id_Arg_List();
            if (!Char(']')) {
              throw exception::eval_error("Incomplete anonymous function bind", File_Position(m_position.line, m_position.col), *m_filename);
            }
          } else {
            // make sure we always have the same number of nodes
            build_match<eval::Arg_List_AST_Node<Tracer>>(prev_stack_top);
          }

          if (Char('(')) {
            Decl_Arg_List();
            if (!Char(')')) {
              throw exception::eval_error("Incomplete anonymous function", File_Position(m_position.line, m_position.col), *m_filename);
            }
          } else {
            throw exception::eval_error("Incomplete anonymous function", File_Position(m_position.line, m_position.col), *m_filename);
          }


          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete anonymous function", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Lambda_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads a function definition from input
      bool Def(const bool t_class_context = false, const std::string &t_class_name = "") {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("def")) {
          retval = true;

          if (t_class_context) {
            m_match_stack.push_back(make_node<eval::Id_AST_Node<Tracer>>(t_class_name, m_position.line, m_position.col));
          }

          if (!Id(true)) {
            throw exception::eval_error("Missing function name in definition", File_Position(m_position.line, m_position.col), *m_filename);
          }

          bool is_method = false;

          if (Symbol("::")) {
            //We're now a method
            is_method = true;

            if (!Id(true)) {
              throw exception::eval_error("Missing method name in definition", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }

          if (Char('(')) {
            Decl_Arg_List();
            if (!Char(')')) {
              throw exception::eval_error("Incomplete function definition", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }

          while (Eos()) {}

          if (Char(':')) {
            if (!Operator()) {
              throw exception::eval_error("Missing guard expression for function", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }

          while (Eol()) {}
          if (!Block()) {
            throw exception::eval_error("Incomplete function definition", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (is_method || t_class_context) {
            build_match<eval::Method_AST_Node<Tracer>>(prev_stack_top);
          } else {
            build_match<eval::Def_AST_Node<Tracer>>(prev_stack_top);
          }

        }

        return retval;
      }

      /// Reads a function definition from input
      bool Try() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("try")) {
          retval = true;

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'try' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          bool has_matches = true;
          while (has_matches) {
            while (Eol()) {}
            has_matches = false;
            if (Keyword("catch")) {
              const auto catch_stack_top = m_match_stack.size();
              if (Char('(')) {
                if (!(Arg() && Char(')'))) {
                  throw exception::eval_error("Incomplete 'catch' expression", File_Position(m_position.line, m_position.col), *m_filename);
                }
                if (Char(':')) {
                  if (!Operator()) {
                    throw exception::eval_error("Missing guard expression for catch", File_Position(m_position.line, m_position.col), *m_filename);
                  }
                }
              }

              while (Eol()) {}

              if (!Block()) {
                throw exception::eval_error("Incomplete 'catch' block", File_Position(m_position.line, m_position.col), *m_filename);
              }
              build_match<eval::Catch_AST_Node<Tracer>>(catch_stack_top);
              has_matches = true;
            }
          }
          while (Eol()) {}
          if (Keyword("finally")) {
            const auto finally_stack_top = m_match_stack.size();

            while (Eol()) {}

            if (!Block()) {
              throw exception::eval_error("Incomplete 'finally' block", File_Position(m_position.line, m_position.col), *m_filename);
            }
            build_match<eval::Finally_AST_Node<Tracer>>(finally_stack_top);
          }

          build_match<eval::Try_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads an if/else if/else block from input
      bool If() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("if")) {
          retval = true;

          if (!Char('(')) {
            throw exception::eval_error("Incomplete 'if' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (!Equation()) {
            throw exception::eval_error("Incomplete 'if' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          const bool is_if_init = Eol() && Equation();

          if (!Char(')')) {
            throw exception::eval_error("Incomplete 'if' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'if' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          bool has_matches = true;
          while (has_matches) {
            while (Eol()) {}
            has_matches = false;
            if (Keyword("else")) {
              if (If()) {
                has_matches = true;
              } else {
                while (Eol()) {}

                if (!Block()) {
                  throw exception::eval_error("Incomplete 'else' block", File_Position(m_position.line, m_position.col), *m_filename);
                }
                has_matches = true;
              }
            }
          }

          const auto num_children = m_match_stack.size() - prev_stack_top;

          if ((is_if_init && num_children == 3)
              || (!is_if_init && num_children == 2)) {
            m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
          }

          if (!is_if_init) {
            build_match<eval::If_AST_Node<Tracer>>(prev_stack_top);
          } else {
            build_match<eval::If_AST_Node<Tracer>>(prev_stack_top+1);
            build_match<eval::Block_AST_Node<Tracer>>(prev_stack_top);
          }
        }

        return retval;
      }

      /// Reads a class block from input
      bool Class(const bool t_class_allowed) {
        Depth_Counter dc{this};
        bool retval = false;

        size_t prev_stack_top = m_match_stack.size();

        if (Keyword("class")) {
          if (!t_class_allowed) {
            throw exception::eval_error("Class definitions only allowed at top scope", File_Position(m_position.line, m_position.col), *m_filename);
          }

          retval = true;

          if (!Id(true)) {
            throw exception::eval_error("Missing class name in definition", File_Position(m_position.line, m_position.col), *m_filename);
          }

          const auto class_name = m_match_stack.back()->text;

          while (Eol()) {}

          if (!Class_Block(class_name)) {
            throw exception::eval_error("Incomplete 'class' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Class_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }


      /// Reads a while block from input
      bool While() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("while")) {
          retval = true;

          if (!Char('(')) {
            throw exception::eval_error("Incomplete 'while' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (!(Operator() && Char(')'))) {
            throw exception::eval_error("Incomplete 'while' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'while' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::While_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads the ranged `for` conditions from input
      bool Range_Expression() {
        Depth_Counter dc{this};
        // the first element will have already been captured by the For_Guards() call that preceeds it
        return Char(':') && Equation();
      }


      /// Reads the C-style `for` conditions from input
      bool For_Guards() {
        Depth_Counter dc{this};
        if (!(Equation() && Eol()))
        {
          if (!Eol())
          {
            return false;
          } else {
            m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
          }
        }

        if (!(Equation() && Eol()))
        {
          if (!Eol())
          {
            return false;
          } else {
            m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Constant_AST_Node<Tracer>>(Boxed_Value(true)));
          }
        }

        if (!Equation())
        {
          m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
        }

        return true; 
      }


      /// Reads a for block from input
      bool For() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("for")) {
          retval = true;

          if (!Char('(')) {
            throw exception::eval_error("Incomplete 'for' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          const bool classic_for = For_Guards() && Char(')');
          if (!classic_for && !(Range_Expression() && Char(')'))) {
            throw exception::eval_error("Incomplete 'for' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'for' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          const auto num_children = m_match_stack.size() - prev_stack_top;

          if (classic_for) {
            if (num_children != 4) {
              throw exception::eval_error("Incomplete 'for' expression", File_Position(m_position.line, m_position.col), *m_filename);
            }
            build_match<eval::For_AST_Node<Tracer>>(prev_stack_top);
          } else {
            if (num_children != 3) {
              throw exception::eval_error("Incomplete ranged-for expression", File_Position(m_position.line, m_position.col), *m_filename);
            }
            build_match<eval::Ranged_For_AST_Node<Tracer>>(prev_stack_top);
          }
        }

        return retval;
      }


      /// Reads a case block from input
      bool Case() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("case")) {
          retval = true;

          if (!Char('(')) {
            throw exception::eval_error("Incomplete 'case' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (!(Operator() && Char(')'))) {
            throw exception::eval_error("Incomplete 'case' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'case' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Case_AST_Node<Tracer>>(prev_stack_top);
        } else if (Keyword("default")) {
          retval = true;

          while (Eol()) {}

          if (!Block()) {
            throw exception::eval_error("Incomplete 'default' block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Default_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }


      /// Reads a switch statement from input
      bool Switch() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("switch")) {

          if (!Char('(')) {
            throw exception::eval_error("Incomplete 'switch' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (!(Operator() && Char(')'))) {
            throw exception::eval_error("Incomplete 'switch' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          while (Eol()) {}

          if (Char('{')) {
            while (Eol()) {}

            while (Case()) {
              while (Eol()) { } // eat
            }

            while (Eol()) { } // eat

            if (!Char('}')) {
              throw exception::eval_error("Incomplete block", File_Position(m_position.line, m_position.col), *m_filename);
            }
          }
          else {
            throw exception::eval_error("Incomplete block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Switch_AST_Node<Tracer>>(prev_stack_top);
          return true;

        } else {
          return false;
        }

      }


      /// Reads a curly-brace C-style class block from input
      bool Class_Block(const std::string &t_class_name) {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Char('{')) {
          retval = true;

          Class_Statements(t_class_name);
          if (!Char('}')) {
            throw exception::eval_error("Incomplete class block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (m_match_stack.size() == prev_stack_top) {
            m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
          }

          build_match<eval::Block_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads a curly-brace C-style block from input
      bool Block() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (Char('{')) {
          retval = true;

          Statements();
          if (!Char('}')) {
            throw exception::eval_error("Incomplete block", File_Position(m_position.line, m_position.col), *m_filename);
          }

          if (m_match_stack.size() == prev_stack_top) {
            m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
          }

          build_match<eval::Block_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads a return statement from input
      bool Return() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("return")) {
          Operator();
          build_match<eval::Return_AST_Node<Tracer>>(prev_stack_top);
          return true;
        } else {
          return false;
        }
      }

      /// Reads a break statement from input
      bool Break() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("break")) {
          build_match<eval::Break_AST_Node<Tracer>>(prev_stack_top);
          return true;
        } else {
          return false;
        }
      }

      /// Reads a continue statement from input
      bool Continue() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Keyword("continue")) {
          build_match<eval::Continue_AST_Node<Tracer>>(prev_stack_top);
          return true;
        } else {
          return false;
        }
      }

      /// Reads a dot expression(member access), then proceeds to check if it's a function or array call
      bool Dot_Fun_Array() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();
        if (Lambda() || Num() || Quoted_String() || Single_Quoted_String() ||
            Paren_Expression() || Inline_Container() || Id(false))
        {
          retval = true;
          bool has_more = true;

          while (has_more) {
            has_more = false;

            if (Char('(')) {
              has_more = true;

              Arg_List();
              if (!Char(')')) {
                throw exception::eval_error("Incomplete function call", File_Position(m_position.line, m_position.col), *m_filename);
              }

              build_match<eval::Fun_Call_AST_Node<Tracer>>(prev_stack_top);
              /// \todo Work around for method calls until we have a better solution
              if (!m_match_stack.back()->children.empty()) {
                if (m_match_stack.back()->children[0]->identifier == AST_Node_Type::Dot_Access) {
                  if (m_match_stack.empty()) { throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
}
                  if (m_match_stack.back()->children.empty()) { throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
}
                  auto dot_access = std::move(m_match_stack.back()->children[0]);
                  auto func_call = std::move(m_match_stack.back());
                  m_match_stack.pop_back();
                  func_call->children.erase(func_call->children.begin());
                  if (dot_access->children.empty()) { throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
}
                  func_call->children.insert(func_call->children.begin(), std::move(dot_access->children.back()));
                  dot_access->children.pop_back();
                  dot_access->children.push_back(std::move(func_call));
                  if (dot_access->children.size() != 2) { throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
}
                  m_match_stack.push_back(std::move(dot_access));
                }
              }
            } else if (Char('[')) {
              has_more = true;

              if (!(Operator() && Char(']'))) {
                throw exception::eval_error("Incomplete array access", File_Position(m_position.line, m_position.col), *m_filename);
              }

              build_match<eval::Array_Call_AST_Node<Tracer>>(prev_stack_top);
            }
            else if (Symbol(".")) {
              has_more = true;
              if (!(Id(true))) {
                throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
              }

              if ( std::distance(m_match_stack.begin() + static_cast<int>(prev_stack_top), m_match_stack.end()) != 2) {
                throw exception::eval_error("Incomplete dot access fun call", File_Position(m_position.line, m_position.col), *m_filename);
              }
              build_match<eval::Dot_Access_AST_Node<Tracer>>(prev_stack_top);
            }
          }
        }

        return retval;
      }

      /// Reads a variable declaration from input
      bool Var_Decl(const bool t_class_context = false, const std::string &t_class_name = "") {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();

        if (t_class_context && (Keyword("attr") || Keyword("auto") || Keyword("var"))) {
          retval = true;

          m_match_stack.push_back(make_node<eval::Id_AST_Node<Tracer>>(t_class_name, m_position.line, m_position.col));

          if (!Id(true)) {
            throw exception::eval_error("Incomplete attribute declaration", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Attr_Decl_AST_Node<Tracer>>(prev_stack_top);
        } else if (Keyword("auto") || Keyword("var") ) {
          retval = true;

          if (Reference()) {
            // we built a reference node - continue
          } else if (Id(true)) {
            build_match<eval::Var_Decl_AST_Node<Tracer>>(prev_stack_top);
          } else {
            throw exception::eval_error("Incomplete variable declaration", File_Position(m_position.line, m_position.col), *m_filename);
          }

        } else if (Keyword("global")) {
          retval = true;

          if (!(Reference() || Id(true))) {
            throw exception::eval_error("Incomplete global declaration", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Global_Decl_AST_Node<Tracer>>(prev_stack_top);
        } else if (Keyword("attr")) {
          retval = true;

          if (!Id(true)) {
            throw exception::eval_error("Incomplete attribute declaration", File_Position(m_position.line, m_position.col), *m_filename);
          }
          if (!Symbol("::")) {
            throw exception::eval_error("Incomplete attribute declaration", File_Position(m_position.line, m_position.col), *m_filename);
          }
          if (!Id(true)) {
            throw exception::eval_error("Missing attribute name in definition", File_Position(m_position.line, m_position.col), *m_filename);
          }


          build_match<eval::Attr_Decl_AST_Node<Tracer>>(prev_stack_top);
        }

        return retval;
      }

      /// Reads an expression surrounded by parentheses from input
      bool Paren_Expression() {
        Depth_Counter dc{this};
        if (Char('(')) {
          if (!Operator()) {
            throw exception::eval_error("Incomplete expression", File_Position(m_position.line, m_position.col), *m_filename);
          }
          if (!Char(')')) {
            throw exception::eval_error("Missing closing parenthesis ')'", File_Position(m_position.line, m_position.col), *m_filename);
          }
          return true;
        } else {
          return false;
        }
      }

      /// Reads, and identifies, a short-form container initialization from input
      bool Inline_Container() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Char('[')) {
          Container_Arg_List();

          if (!Char(']')) {
            throw exception::eval_error("Missing closing square bracket ']' in container initializer", File_Position(m_position.line, m_position.col), *m_filename);
          }
          if ((prev_stack_top != m_match_stack.size()) && (!m_match_stack.back()->children.empty())) {
            if (m_match_stack.back()->children[0]->identifier == AST_Node_Type::Value_Range) {
              build_match<eval::Inline_Range_AST_Node<Tracer>>(prev_stack_top);
            }
            else if (m_match_stack.back()->children[0]->identifier == AST_Node_Type::Map_Pair) {
              build_match<eval::Inline_Map_AST_Node<Tracer>>(prev_stack_top);
            }
            else {
              build_match<eval::Inline_Array_AST_Node<Tracer>>(prev_stack_top);
            }
          }
          else {
            build_match<eval::Inline_Array_AST_Node<Tracer>>(prev_stack_top);
          }

          return true;
        } else {
          return false;
        }
      }

      /// Parses a variable specified with a & aka reference
      bool Reference() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        if (Symbol("&")) {
          if (!Id(true)) {
            throw exception::eval_error("Incomplete '&' expression", File_Position(m_position.line, m_position.col), *m_filename);
          }

          build_match<eval::Reference_AST_Node<Tracer>>(prev_stack_top);
          return true;
        } else {
          return false;
        }
      }

      /// Reads a unary prefixed expression from input
      bool Prefix() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();
        using SS = utility::Static_String;
        constexpr const std::array<utility::Static_String, 6> prefix_opers{{
            SS{"++"}, 
            SS{"--"}, 
            SS{"-"}, 
            SS{"+"}, 
            SS{"!"}, 
            SS{"~"}
        }};

        for (const auto &oper : prefix_opers)
        {
          const bool is_char = oper.size() == 1;
          if ((is_char && Char(oper.c_str()[0])) || (!is_char && Symbol(oper)))
          {
            if (!Operator(m_operators.size()-1)) {
              throw exception::eval_error("Incomplete prefix '" + std::string(oper.c_str()) + "' expression", File_Position(m_position.line, m_position.col), *m_filename);
            }

            build_match<eval::Prefix_AST_Node<Tracer>>(prev_stack_top, oper.c_str());
            return true;
          }
        }

        return false;
      }

      /// Parses any of a group of 'value' style ast_node groups from input
      bool Value() {
        Depth_Counter dc{this};
        return Var_Decl() || Dot_Fun_Array() || Prefix();
      }

      bool Operator_Helper(const size_t t_precedence, std::string &oper) {
        Depth_Counter dc{this};
        for (auto & elem : m_operator_matches[t_precedence]) {
          if (Symbol(elem)) {
            oper = elem.c_str();
            return true;
          }
        }
        return false;
      }

      bool Operator(const size_t t_precedence = 0) {
        Depth_Counter dc{this};
        bool retval = false;
        const auto prev_stack_top = m_match_stack.size();

        if (m_operators[t_precedence] != Operator_Precidence::Prefix) {
          if (Operator(t_precedence+1)) {
            retval = true;
            std::string oper;
            while (Operator_Helper(t_precedence, oper)) {
              while (Eol()) {}
              if (!Operator(t_precedence+1)) {
                throw exception::eval_error("Incomplete '" + oper + "' expression",
                    File_Position(m_position.line, m_position.col), *m_filename);
              }

              switch (m_operators[t_precedence]) {
                case(Operator_Precidence::Ternary_Cond) :
                  if (Symbol(":")) {
                    if (!Operator(t_precedence+1)) {
                      throw exception::eval_error("Incomplete '" + oper + "' expression",
                          File_Position(m_position.line, m_position.col), *m_filename);
                    }
                    build_match<eval::If_AST_Node<Tracer>>(prev_stack_top);
                  }
                  else {
                    throw exception::eval_error("Incomplete '" + oper + "' expression",
                        File_Position(m_position.line, m_position.col), *m_filename);
                  }
                  break;

                case(Operator_Precidence::Addition) :
                case(Operator_Precidence::Multiplication) :
                case(Operator_Precidence::Shift) :
                case(Operator_Precidence::Equality) :
                case(Operator_Precidence::Bitwise_And) :
                case(Operator_Precidence::Bitwise_Xor) :
                case(Operator_Precidence::Bitwise_Or) :
                case(Operator_Precidence::Comparison) :
                  build_match<eval::Binary_Operator_AST_Node<Tracer>>(prev_stack_top, oper);
                  break;

                case(Operator_Precidence::Logical_And) :
                  build_match<eval::Logical_And_AST_Node<Tracer>>(prev_stack_top, oper);
                  break;
                case(Operator_Precidence::Logical_Or) :
                  build_match<eval::Logical_Or_AST_Node<Tracer>>(prev_stack_top, oper);
                  break;
                case(Operator_Precidence::Prefix) :
                  assert(false); // cannot reach here because of if() statement at the top
                  break;

//                default:
//                  throw exception::eval_error("Internal error: unhandled ast_node", File_Position(m_position.line, m_position.col), *m_filename);
              }
            }
          }
        } else {
          return Value();
        }

        return retval;
      }

      /// Reads a pair of values used to create a map initialization from input
      bool Map_Pair() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();
        const auto prev_pos = m_position;

        if (Operator()) {
          if (Symbol(":")) {
            retval = true;
            if (!Operator()) {
              throw exception::eval_error("Incomplete map pair", File_Position(m_position.line, m_position.col), *m_filename);
            }

            build_match<eval::Map_Pair_AST_Node<Tracer>>(prev_stack_top);
          }
          else {
            m_position = prev_pos;
            while (prev_stack_top != m_match_stack.size()) {
              m_match_stack.pop_back();
            }
          }
        }

        return retval;
      }

      /// Reads a pair of values used to create a range initialization from input
      bool Value_Range() {
        Depth_Counter dc{this};
        bool retval = false;

        const auto prev_stack_top = m_match_stack.size();
        const auto prev_pos = m_position;

        if (Operator()) {
          if (Symbol("..")) {
            retval = true;
            if (!Operator()) {
              throw exception::eval_error("Incomplete value range", File_Position(m_position.line, m_position.col), *m_filename);
            }

            build_match<eval::Value_Range_AST_Node<Tracer>>(prev_stack_top);
          }
          else {
            m_position = prev_pos;
            while (prev_stack_top != m_match_stack.size()) {
              m_match_stack.pop_back();
            }
          }
        }

        return retval;
      }

      /// Parses a string of binary equation operators
      bool Equation() {
        Depth_Counter dc{this};
        const auto prev_stack_top = m_match_stack.size();

        using SS = utility::Static_String;

        if (Operator()) {
          for (const auto &sym : {SS{"="}, SS{":="}, SS{"+="}, SS{"-="}, SS{"*="}, SS{"/="}, SS{"%="}, SS{"<<="}, SS{">>="}, SS{"&="}, SS{"^="}, SS{"|="}}) 
          {
            if (Symbol(sym, true)) {
              SkipWS(true);
              if (!Equation()) {
                throw exception::eval_error("Incomplete equation", File_Position(m_position.line, m_position.col), *m_filename);
              }

              build_match<eval::Equation_AST_Node<Tracer>>(prev_stack_top, sym.c_str());
              return true;
            }
          }
          return true;
        }

        return false;
      }

      /// Parses statements allowed inside of a class block
      bool Class_Statements(const std::string &t_class_name) {
        Depth_Counter dc{this};
        bool retval = false;

        bool has_more = true;
        bool saw_eol = true;

        while (has_more) {
          const auto start = m_position;
          if (Def(true, t_class_name) || Var_Decl(true, t_class_name)) {
            if (!saw_eol) {
              throw exception::eval_error("Two function definitions missing line separator", File_Position(start.line, start.col), *m_filename);
            }
            has_more = true;
            retval = true;
            saw_eol = true;
          } else if (Eol()) {
            has_more = true;
            retval = true;
            saw_eol = true;
          } else {
            has_more = false;
          }
        }

        return retval;
      }

      /// Top level parser, starts parsing of all known parses
      bool Statements(const bool t_class_allowed = false) {
        Depth_Counter dc{this};
        bool retval = false;

        bool has_more = true;
        bool saw_eol = true;

        while (has_more) {
          const auto start = m_position;
          if (Def() || Try() || If() || While() || Class(t_class_allowed) || For() || Switch()) {
            if (!saw_eol) {
              throw exception::eval_error("Two function definitions missing line separator", File_Position(start.line, start.col), *m_filename);
            }
            has_more = true;
            retval = true;
            saw_eol = true;
          }
          else if (Return() || Break() || Continue() || Equation()) {
            if (!saw_eol) {
              throw exception::eval_error("Two expressions missing line separator", File_Position(start.line, start.col), *m_filename);
            }
            has_more = true;
            retval = true;
            saw_eol = false;
          }
          else if (Block() || Eol()) {
            has_more = true;
            retval = true;
            saw_eol = true;
          }
          else {
            has_more = false;
          }
        }

        return retval;
      }

      AST_NodePtr parse(const std::string &t_input, const std::string &t_fname) override
      {
        ChaiScript_Parser<Tracer, Optimizer> parser(m_tracer, m_optimizer);
        return parser.parse_internal(t_input, t_fname);
      }

      eval::AST_Node_Impl_Ptr<Tracer> parse_instr_eval(const std::string &t_input)
      {
        auto last_position    = m_position;
        auto last_filename    = m_filename;
        auto last_match_stack = std::exchange(m_match_stack, decltype(m_match_stack){});

        auto retval = parse_internal(t_input, "instr eval");

        m_position = std::move(last_position);
        m_filename = std::move(last_filename);
        m_match_stack = std::move(last_match_stack);

        return eval::AST_Node_Impl_Ptr<Tracer>(dynamic_cast<eval::AST_Node_Impl<Tracer>*>(retval.release()));
      }

      /// Parses the given input string, tagging parsed ast_nodes with the given m_filename.
      AST_NodePtr parse_internal(const std::string &t_input, std::string t_fname) {
        m_position = Position(t_input.begin(), t_input.end());
        m_filename = std::make_shared<std::string>(std::move(t_fname));

        if ((t_input.size() > 1) && (t_input[0] == '#') && (t_input[1] == '!')) {
          while (m_position.has_more() && (!Eol())) {
            ++m_position;
          }
        }

        if (Statements(true)) {
          if (m_position.has_more()) {
            throw exception::eval_error("Unparsed input", File_Position(m_position.line, m_position.col), *m_filename);
          } else {
            build_match<eval::File_AST_Node<Tracer>>(0);
          }
        } else {
          m_match_stack.push_back(chaiscript::make_unique<eval::AST_Node_Impl<Tracer>, eval::Noop_AST_Node<Tracer>>());
        }

        AST_NodePtr retval(std::move(m_match_stack.front()));
        m_match_stack.clear();
        return retval;
      }
    };
  }
}

#if defined(CHAISCRIPT_MSVC) && defined(CHAISCRIPT_PUSHED_MIN_MAX)
#undef CHAISCRIPT_PUSHED_MIN_MAX
#pragma pop_macro("min")
#pragma pop_macro("max")
#endif


#endif /* CHAISCRIPT_PARSER_HPP_ */

