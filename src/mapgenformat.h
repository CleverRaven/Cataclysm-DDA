#ifndef MAPGENFORMAT_H
#define MAPGENFORMAT_H

#include <vector>
#include <memory>

#include "map.h"
/////

struct ter_furn_id;

void formatted_set_incredibly_simple(
  map * m, const ter_furn_id data[], int width, int height, int startx, int starty, ter_id defter
);

/////
namespace mapf
{
 namespace internal
 {
  class format_effect;
 }
/** The return statement for this method is not finalized.
 * The following things will be acces from this return statements.
 * - the region of points set by given character
 * - the region of points set to a given terrian id
 * - possibly some other stuff
 * You will have specify the values you want to track with a parameter.
 */
void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       std::shared_ptr<internal::format_effect> ter_b, std::shared_ptr<internal::format_effect> furn_b,
                       const bool empty_toilets = false);

std::shared_ptr<internal::format_effect> basic_bind(std::string characters, ...);
std::shared_ptr<internal::format_effect> ter_str_bind(std::string characters, ...);
std::shared_ptr<internal::format_effect> furn_str_bind(std::string characters, ...);
std::shared_ptr<internal::format_effect> simple_method_bind(std::string characters, ...);

// Anything specified in here isn't finalized
namespace internal
{
 class determine_terrain;
 struct format_data
 {
  std::map<char, std::shared_ptr<determine_terrain> > bindings;
  bool fix_bindings(const char c);
 };

 // This class will become an interface in the future.
 class format_effect
 {
  private:
  std::string characters;
  std::vector< std::shared_ptr<determine_terrain> > determiners;

  public:
   format_effect(std::string characters, std::vector<std::shared_ptr<determine_terrain> > &determiners);
   virtual ~format_effect();

   void execute(format_data& data);
 };

 class determine_terrain
 {
  public:
   virtual ~determine_terrain() {}
   virtual int operator ()(map* m, const int x, const int y) = 0;
 };

    class statically_determine_terrain : public determine_terrain
    {
    private:
        int id;
    public:
        statically_determine_terrain() : id(0) {}
        statically_determine_terrain(int pid) : id(pid) {}
        virtual ~statically_determine_terrain() {}
        virtual int operator ()(map *, const int /*x*/, const int /*y*/) override {
            return id;
        }
    };

    class determine_terrain_with_simple_method : public determine_terrain
    {
    public:
        typedef ter_id (*ter_id_func)();
    private:
        ter_id_func f;
    public:
        determine_terrain_with_simple_method() : f(NULL) {}
        determine_terrain_with_simple_method(ter_id_func pf) : f(pf) {}
        virtual ~determine_terrain_with_simple_method() {}
        virtual int operator ()(map *, const int /*x*/, const int /*y*/) override {
            return f();
        }
    };

 //TODO: make use of this
 class determine_terrain_with_complex_method : public determine_terrain
 {
  private:
   ter_id (*f)(map*, const int, const int);
  public:
   determine_terrain_with_complex_method():f(NULL) {}
   determine_terrain_with_complex_method(ter_id (*pf)(map*, const int, const int)):f(pf) {}
   virtual ~determine_terrain_with_complex_method() {}
   virtual int operator ()(map* m, const int x, const int y) override{return f(m,x,y);}
 };


} //END NAMESPACE mapf::internal

} //END NAMESPACE mapf

#endif
