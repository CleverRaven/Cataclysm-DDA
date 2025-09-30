#ifndef CHAISCRIPT_SIMPLEJSON_WRAP_HPP
#define CHAISCRIPT_SIMPLEJSON_WRAP_HPP

#include "json.hpp"

namespace chaiscript
{
  class json_wrap
  {
    public:

      static Module& library(Module& m)
      {

        m.add(chaiscript::fun([](const std::string &t_str) { return from_json(t_str); }), "from_json");
        m.add(chaiscript::fun(&json_wrap::to_json), "to_json");

        return m;

      }

    private:

      static Boxed_Value from_json(const json::JSON &t_json)
      {
        switch( t_json.JSONType() ) {
          case json::JSON::Class::Null:
            return Boxed_Value();
          case json::JSON::Class::Object:
            {
              std::map<std::string, Boxed_Value> m;

              for (const auto &p : t_json.object_range())
              {
                m.insert(std::make_pair(p.first, from_json(p.second)));
              }

              return Boxed_Value(m);
            }
          case json::JSON::Class::Array:
            {
              std::vector<Boxed_Value> vec;

              for (const auto &p : t_json.array_range()) 
              {
                vec.emplace_back(from_json(p));
              }

              return Boxed_Value(vec);
            }
          case json::JSON::Class::String:
            return Boxed_Value(t_json.to_string());
          case json::JSON::Class::Floating:
            return Boxed_Value(t_json.to_float());
          case json::JSON::Class::Integral:
            return Boxed_Value(t_json.to_int());
          case json::JSON::Class::Boolean:
            return Boxed_Value(t_json.to_bool());
        }

        throw std::runtime_error("Unknown JSON type");
      }

      static Boxed_Value from_json(const std::string &t_json)
      {
        try {
          return from_json( json::JSON::Load(t_json) );
        } catch (const std::out_of_range& ) {
          throw std::runtime_error("Unparsed JSON input");
        }
      }

      static std::string to_json(const Boxed_Value &t_bv)
      {
        return to_json_object(t_bv).dump();
      }

      static json::JSON to_json_object(const Boxed_Value &t_bv)
      {
        try {
          const std::map<std::string, Boxed_Value> m = chaiscript::boxed_cast<const std::map<std::string, Boxed_Value> &>(t_bv);

          json::JSON obj(json::JSON::Class::Object);
          for (const auto &o : m)
          {
            obj[o.first] = to_json_object(o.second);
          }
          return obj;
        } catch (const chaiscript::exception::bad_boxed_cast &) {
          // not a map
        }

        try {
          const std::vector<Boxed_Value> v = chaiscript::boxed_cast<const std::vector<Boxed_Value> &>(t_bv);

          json::JSON obj(json::JSON::Class::Array);
          for (size_t i = 0; i < v.size(); ++i)
          {
            obj[i] = to_json_object(v[i]);
          }
          return obj;
        } catch (const chaiscript::exception::bad_boxed_cast &) {
          // not a vector
        }


        try {
          Boxed_Number bn(t_bv);
          if (Boxed_Number::is_floating_point(t_bv))
          {
            return json::JSON(bn.get_as<double>());
          } else {
            return json::JSON(bn.get_as<std::int64_t>());
          }
        } catch (const chaiscript::detail::exception::bad_any_cast &) {
          // not a number
        }

        try {
          return json::JSON(boxed_cast<bool>(t_bv));
        } catch (const chaiscript::exception::bad_boxed_cast &) {
          // not a bool
        }

        try {
          return json::JSON(boxed_cast<std::string>(t_bv));
        } catch (const chaiscript::exception::bad_boxed_cast &) {
          // not a string
        }


        try {
          const chaiscript::dispatch::Dynamic_Object &o = boxed_cast<const dispatch::Dynamic_Object &>(t_bv);

          json::JSON obj(json::JSON::Class::Object);
          for (const auto &attr : o.get_attrs())
          {
            obj[attr.first] = to_json_object(attr.second);
          }
          return obj;
        } catch (const chaiscript::exception::bad_boxed_cast &) {
          // not a dynamic object
        }

        if (t_bv.is_null()) return json::JSON(); // a null value

        throw std::runtime_error("Unknown object type to convert to JSON");
      }


  };


}

#endif
