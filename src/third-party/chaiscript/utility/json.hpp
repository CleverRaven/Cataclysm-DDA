// From github.com/nbsdx/SimpleJSON. 
// Released under the DWTFYW PL
//


#pragma once

#ifndef SIMPLEJSON_HPP
#define SIMPLEJSON_HPP


#include <cstdint>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <type_traits>
#include <initializer_list>
#include <ostream>
#include <iostream>
#include "../chaiscript_defines.hpp"

namespace json {

using std::enable_if;
using std::initializer_list;
using std::is_same;
using std::is_convertible;
using std::is_integral;
using std::is_floating_point;




class JSON
{
  public:
    enum class Class {
      Null,
      Object,
      Array,
      String,
      Floating,
      Integral,
      Boolean
    };

  private:

    struct QuickFlatMap
    {
      auto find(const std::string &s) {
        return std::find_if(std::begin(data), std::end(data), [&s](const auto &d) { return d.first == s; });
      }

      auto find(const std::string &s) const {
        return std::find_if(std::begin(data), std::end(data), [&s](const auto &d) { return d.first == s; });
      }

      auto size() const {
        return data.size();
      }

      auto begin() const {
        return data.begin();
      }

      auto end() const {
        return data.end();
      }


      auto begin() {
        return data.begin();
      }

      auto end() {
        return data.end();
      }


      JSON &operator[](const std::string &s) {
        const auto itr = find(s);
        if (itr != data.end()) {
          return itr->second;
        } else {
          data.emplace_back(s, JSON());
          return data.back().second;
        }
      }

      JSON &at(const std::string &s) {
        const auto itr = find(s);
        if (itr != data.end()) {
          return itr->second;
        } else {
          throw std::out_of_range("Unknown key: " + s);
        }
      }

      const JSON &at(const std::string &s) const {
        const auto itr = find(s);
        if (itr != data.end()) {
          return itr->second;
        } else {
          throw std::out_of_range("Unknown key: " + s);
        }
      }

      size_t count(const std::string &s) const {
        return (find(s) != data.end())?1:0;
      }

      std::vector<std::pair<std::string, JSON>> data;

      using iterator = decltype(data)::iterator;
      using const_iterator = decltype(data)::const_iterator;


    };

    struct Internal {
      template<typename T>
        auto clone(const std::unique_ptr<T> &ptr) {
          if (ptr != nullptr) {
            return std::make_unique<T>(*ptr);
          } else {
            return std::unique_ptr<T>(nullptr);
          }
        }

      Internal( double d ) : Float( d ), Type(Class::Floating) {}
      Internal( int64_t l ) : Int( l ), Type(Class::Integral) {}
      Internal( bool   b ) : Bool( b ), Type(Class::Boolean) {}
      Internal( std::string s ) : String(std::make_unique<std::string>(std::move(s))), Type(Class::String) {}
      Internal()           : Type(Class::Null) {}

      Internal(Class t_type) {
        set_type(t_type);
      }

      Internal(const Internal &other)
        : List(clone(other.List)),
        Map(clone(other.Map)),
        String(clone(other.String)),
        Float(other.Float),
        Int(other.Int),
        Bool(other.Bool),
        Type(other.Type)
      {
      }

      Internal &operator=(const Internal &other)
      {
        List = clone(other.List);
        Map = clone(other.Map);
        String = clone(other.String);
        Float = other.Float;
        Int = other.Int;
        Bool = other.Bool;
        Type = other.Type;
        return *this;
      }

      void set_type( Class type ) {
        if( type == Type ) {
          return;
        }

        Map.reset();
        List.reset();
        String.reset();

        switch( type ) {
          case Class::Object:    Map    = std::make_unique<QuickFlatMap>(); break;
          case Class::Array:     List   = std::make_unique<std::vector<JSON>>();      break;
          case Class::String:    String = std::make_unique<std::string>();           break;
          case Class::Floating:  Float  = 0.0;                    break;
          case Class::Integral:  Int    = 0;                      break;
          case Class::Boolean:   Bool   = false;                  break;
          case Class::Null:      break;
        }

        Type = type;
      }

      Internal(Internal &&) = default;
      Internal &operator=(Internal &&) = default;

      std::unique_ptr<std::vector<JSON>>      List;
      std::unique_ptr<QuickFlatMap> Map;
      std::unique_ptr<std::string>           String;
      double              Float = 0;
      int64_t             Int = 0;
      bool                Bool = false;

      Class Type = Class::Null;
    };

    Internal internal;

  public:

    template <typename Container>
      class JSONWrapper {
        Container *object = nullptr;

        public:
        JSONWrapper( Container *val ) : object( val ) {}
        JSONWrapper( std::nullptr_t ) {}

        typename Container::iterator begin() { return object ? object->begin() : typename Container::iterator(); }
        typename Container::iterator end() { return object ? object->end() : typename Container::iterator(); }
        typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::iterator(); }
        typename Container::const_iterator end() const { return object ? object->end() : typename Container::iterator(); }
      };

    template <typename Container>
      class JSONConstWrapper {
        const Container *object = nullptr;

        public:
        JSONConstWrapper( const Container *val ) : object( val ) {}
        JSONConstWrapper( std::nullptr_t ) {}

        typename Container::const_iterator begin() const { return object ? object->begin() : typename Container::const_iterator(); }
        typename Container::const_iterator end() const { return object ? object->end() : typename Container::const_iterator(); }
      };

    JSON() = default;
    JSON( std::nullptr_t ) {} 

    explicit JSON(Class type)
      : internal(type)
    {
    }

    JSON( initializer_list<JSON> list ) 
      : internal(Class::Object)
    {
      for( auto i = list.begin(), e = list.end(); i != e; ++i, ++i ) {
        operator[]( i->to_string() ) = *std::next( i );
      }
    }

    template <typename T>
      explicit JSON( T b, typename enable_if<is_same<T,bool>::value>::type* = nullptr ) : internal( static_cast<bool>(b) ) {}

    template <typename T>
      explicit JSON( T i, typename enable_if<is_integral<T>::value && !is_same<T,bool>::value>::type* = nullptr ) : internal( static_cast<int64_t>(i) ) {}

    template <typename T>
      explicit JSON( T f, typename enable_if<is_floating_point<T>::value>::type* = nullptr ) : internal( static_cast<double>(f) ) {}

    template <typename T>
      explicit JSON( T s, typename enable_if<is_convertible<T,std::string>::value>::type* = nullptr ) : internal( static_cast<std::string>(s) ) {}



    static JSON Load( const std::string & );

    JSON& operator[]( const std::string &key ) {
      internal.set_type( Class::Object ); 
      return internal.Map->operator[]( key );
    }

    JSON& operator[]( const size_t index ) {
      internal.set_type( Class::Array );
      if( index >= internal.List->size() ) {
        internal.List->resize( index + 1 );
      }

      return internal.List->operator[]( index );
    }


    JSON &at( const std::string &key ) {
      return operator[]( key );
    }

    const JSON &at( const std::string &key ) const {
      return internal.Map->at( key );
    }

    JSON &at( size_t index ) {
      return operator[]( index );
    }

    const JSON &at( size_t index ) const {
      return internal.List->at( index );
    }


    long length() const {
      if( internal.Type == Class::Array ) {
        return static_cast<long>(internal.List->size());
      } else {
        return -1;
      }
    }

    bool has_key( const std::string &key ) const {
      if( internal.Type == Class::Object ) {
        return internal.Map->count(key) != 0;
      }

      return false;
    }

    int size() const {
      if( internal.Type == Class::Object ) {
        return static_cast<int>(internal.Map->size());
      } else if( internal.Type == Class::Array ) {
        return static_cast<int>(internal.List->size());
      } else {
        return -1;
      }
    }

    Class JSONType() const { return internal.Type; }

    /// Functions for getting primitives from the JSON object.
    bool is_null() const { return internal.Type == Class::Null; }

    std::string to_string() const { bool b; return to_string( b ); }
    std::string to_string( bool &ok ) const {
      ok = (internal.Type == Class::String);
      return ok ? *internal.String : std::string("");
    }

    double to_float() const { bool b; return to_float( b ); }
    double to_float( bool &ok ) const {
      ok = (internal.Type == Class::Floating);
      return ok ? internal.Float : 0.0;
    }

    int64_t to_int() const { bool b; return to_int( b ); }
    int64_t to_int( bool &ok ) const {
      ok = (internal.Type == Class::Integral);
      return ok ? internal.Int : 0;
    }

    bool to_bool() const { bool b; return to_bool( b ); }
    bool to_bool( bool &ok ) const {
      ok = (internal.Type == Class::Boolean);
      return ok ? internal.Bool : false;
    }

    JSONWrapper<QuickFlatMap> object_range() {
      if( internal.Type == Class::Object ) {
        return JSONWrapper<QuickFlatMap>( internal.Map.get() );
      } else {
        return JSONWrapper<QuickFlatMap>( nullptr );
      }
    }

    JSONWrapper<std::vector<JSON>> array_range() {
      if( internal.Type == Class::Array ) {
        return JSONWrapper<std::vector<JSON>>( internal.List.get() );
      } else {
        return JSONWrapper<std::vector<JSON>>( nullptr );
      }
    }

    JSONConstWrapper<QuickFlatMap> object_range() const {
      if( internal.Type == Class::Object ) {
        return JSONConstWrapper<QuickFlatMap>( internal.Map.get() );
      } else {
        return JSONConstWrapper<QuickFlatMap>( nullptr );
      }
    }


    JSONConstWrapper<std::vector<JSON>> array_range() const { 
      if( internal.Type == Class::Array ) {
        return JSONConstWrapper<std::vector<JSON>>( internal.List.get() );
      } else {
        return JSONConstWrapper<std::vector<JSON>>( nullptr );
      }
    }

    std::string dump( long depth = 1, std::string tab = "  ") const {
      switch( internal.Type ) {
        case Class::Null:
          return "null";
        case Class::Object: {
                              std::string pad = "";
                              for( long i = 0; i < depth; ++i, pad += tab ) { }

                              std::string s = "{\n";
                              bool skip = true;
                              for( auto &p : *internal.Map ) {
                                if( !skip ) { s += ",\n"; }
                                s += ( pad + "\"" + json_escape(p.first) + "\" : " + p.second.dump( depth + 1, tab ) );
                                skip = false;
                              }
                              s += ( "\n" + pad.erase( 0, 2 ) + "}" ) ;
                              return s;
                            }
        case Class::Array: {
                             std::string s = "[";
                             bool skip = true;
                             for( auto &p : *internal.List ) {
                               if( !skip ) { s += ", "; }
                               s += p.dump( depth + 1, tab );
                               skip = false;
                             }
                             s += "]";
                             return s;
                           }
        case Class::String:
                           return "\"" + json_escape( *internal.String ) + "\"";
        case Class::Floating:
                           return std::to_string( internal.Float );
        case Class::Integral:
                           return std::to_string( internal.Int );
        case Class::Boolean:
                           return internal.Bool ? "true" : "false";
      }

      throw std::runtime_error("Unhandled JSON type");
    }


  private:
    static std::string json_escape( const std::string &str ) {
      std::string output;
      for(char i : str) {
        switch( i ) {
          case '\"': output += "\\\""; break;
          case '\\': output += "\\\\"; break;
          case '\b': output += "\\b";  break;
          case '\f': output += "\\f";  break;
          case '\n': output += "\\n";  break;
          case '\r': output += "\\r";  break;
          case '\t': output += "\\t";  break;
          default  : output += i; break;
        }
}
      return output;
    }


  private:
};


struct JSONParser {
  static bool isspace(const char c)
  {
#ifdef CHAISCRIPT_MSVC
    // MSVC warns on these line in some circumstances
#pragma warning(push)
#pragma warning(disable : 6330)
#endif
    return ::isspace(c) != 0;
#ifdef CHAISCRIPT_MSVC
#pragma warning(pop)
#endif


  }

  static void consume_ws( const std::string &str, size_t &offset ) {
    while( isspace( str.at(offset) ) && offset <= str.size() ) { ++offset; }
  }

  static JSON parse_object( const std::string &str, size_t &offset ) {
    JSON Object( JSON::Class::Object );

    ++offset;
    consume_ws( str, offset );
    if( str.at(offset) == '}' ) {
      ++offset; return Object;
    }

    for (;offset<str.size();) {
      JSON Key = parse_next( str, offset );
      consume_ws( str, offset );
      if( str.at(offset) != ':' ) {
        throw std::runtime_error(std::string("JSON ERROR: Object: Expected colon, found '") + str.at(offset) + "'\n");
      }
      consume_ws( str, ++offset );
      JSON Value = parse_next( str, offset );
      Object[Key.to_string()] = Value;

      consume_ws( str, offset );
      if( str.at(offset) == ',' ) {
        ++offset; continue;
      }
      else if( str.at(offset) == '}' ) {
        ++offset; break;
      }
      else {
        throw std::runtime_error(std::string("JSON ERROR: Object: Expected comma, found '") + str.at(offset) + "'\n");
      }
    }

    return Object;
  }

  static JSON parse_array( const std::string &str, size_t &offset ) {
    JSON Array( JSON::Class::Array );
    size_t index = 0;

    ++offset;
    consume_ws( str, offset );
    if( str.at(offset) == ']' ) {
      ++offset; return Array;
    }

    for (;offset < str.size();) {
      Array[index++] = parse_next( str, offset );
      consume_ws( str, offset );

      if( str.at(offset) == ',' ) {
        ++offset; continue;
      }
      else if( str.at(offset) == ']' ) {
        ++offset; break;
      }
      else {
        throw std::runtime_error(std::string("JSON ERROR: Array: Expected ',' or ']', found '") + str.at(offset) + "'\n");
      }
    }

    return Array;
  }

  static JSON parse_string( const std::string &str, size_t &offset ) {
    std::string val;
    for( char c = str.at(++offset); c != '\"' ; c = str.at(++offset) ) {
      if( c == '\\' ) {
        switch( str.at(++offset) ) {
          case '\"': val += '\"'; break;
          case '\\': val += '\\'; break;
          case '/' : val += '/' ; break;
          case 'b' : val += '\b'; break;
          case 'f' : val += '\f'; break;
          case 'n' : val += '\n'; break;
          case 'r' : val += '\r'; break;
          case 't' : val += '\t'; break;
          case 'u' : {
                       val += "\\u" ;
                       for( size_t i = 1; i <= 4; ++i ) {
                         c = str.at(offset+i);
                         if( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ) {
                           val += c;
                         } else {
                           throw std::runtime_error(std::string("JSON ERROR: String: Expected hex character in unicode escape, found '") + c + "'");
                         }
                       }
                       offset += 4;
                     } break;
          default  : val += '\\'; break;
        }
      } else {
        val += c;
      }
    }
    ++offset;
    return JSON(val);
  }

  static JSON parse_number( const std::string &str, size_t &offset ) {
    std::string val, exp_str;
    char c = '\0';
    bool isDouble = false;
    bool isNegative = false;
    int64_t exp = 0;
    bool isExpNegative = false;
    if( offset < str.size() && str.at(offset) == '-' ) {
      isNegative = true;
      ++offset;
    }
    for (; offset < str.size() ;) {
      c = str.at(offset++);
      if( c >= '0' && c <= '9' ) {
        val += c;
      } else if( c == '.' && !isDouble ) {
        val += c; 
        isDouble = true;
      } else {
        break;
      }
    }
    if( offset < str.size() && (c == 'E' || c == 'e' )) {
      c = str.at(offset++);
      if( c == '-' ) { 
        isExpNegative = true;
      } else if( c == '+' ) {
        // do nothing
      } else { 
        --offset; 
      }

      for (; offset < str.size() ;) {
        c = str.at(offset++);
        if( c >= '0' && c <= '9' ) {
          exp_str += c;
        } else if( !isspace( c ) && c != ',' && c != ']' && c != '}' ) {
          throw std::runtime_error(std::string("JSON ERROR: Number: Expected a number for exponent, found '") + c + "'");
        }
        else {
          break;
        }
      }
      exp = chaiscript::parse_num<int64_t>( exp_str ) * (isExpNegative?-1:1);
    }
    else if( offset < str.size() && (!isspace( c ) && c != ',' && c != ']' && c != '}' )) {
      throw std::runtime_error(std::string("JSON ERROR: Number: unexpected character '") + c + "'");
    }
    --offset;

    if( isDouble ) {
      return JSON((isNegative?-1:1) * chaiscript::parse_num<double>( val ) * std::pow( 10, exp ));
    } else {
      if( !exp_str.empty() ) {
        return JSON((isNegative?-1:1) * static_cast<double>(chaiscript::parse_num<int64_t>( val )) * std::pow( 10, exp ));
      } else {
        return JSON((isNegative?-1:1) * chaiscript::parse_num<int64_t>( val ));
      }
    }
  }

  static JSON parse_bool( const std::string &str, size_t &offset ) {
    if( str.substr( offset, 4 ) == "true" ) {
      offset += 4;
      return JSON(true);
    } else if( str.substr( offset, 5 ) == "false" ) {
      offset += 5;
      return JSON(false);
    } else {
      throw std::runtime_error(std::string("JSON ERROR: Bool: Expected 'true' or 'false', found '") + str.substr( offset, 5 ) + "'");
    }
  }

  static JSON parse_null( const std::string &str, size_t &offset ) {
    if( str.substr( offset, 4 ) != "null" ) {
      throw std::runtime_error(std::string("JSON ERROR: Null: Expected 'null', found '") + str.substr( offset, 4 ) + "'");
    }
    offset += 4;
    return JSON();
  }

  static JSON parse_next( const std::string &str, size_t &offset ) {
    char value;
    consume_ws( str, offset );
    value = str.at(offset);
    switch( value ) {
      case '[' : return parse_array( str, offset );
      case '{' : return parse_object( str, offset );
      case '\"': return parse_string( str, offset );
      case 't' :
      case 'f' : return parse_bool( str, offset );
      case 'n' : return parse_null( str, offset );
      default  : if( ( value <= '9' && value >= '0' ) || value == '-' ) {
                   return parse_number( str, offset );
                 }
    }
    throw std::runtime_error(std::string("JSON ERROR: Parse: Unexpected starting character '") + value + "'");
  }

};

inline JSON JSON::Load( const std::string &str ) {
  size_t offset = 0;
  return JSONParser::parse_next( str, offset );
}

} // End Namespace json


#endif 
