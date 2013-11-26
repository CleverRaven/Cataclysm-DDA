#ifndef MAPGENFORMAT_H
#define MAPGENFORMAT_H

#include <vector>

#include "map.h"
/////
struct terfurn_tile {
   short ter;
   short furn;
   terfurn_tile();
};

void formatted_set_incredibly_simple( 
  map * m, const terfurn_tile data[], const int width, const int height, const int startx, const int starty, const int defter=-1
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
void formatted_set_terrain(map* m, const int startx, const int starty, const char* cstr, ...);
void formatted_set_simple(map* m, const int startx, const int starty, const char* cstr,
                       internal::format_effect* ter_b, internal::format_effect* furn_b,
                       const bool empty_toilets = false);

internal::format_effect* basic_bind(std::string characters, ...);
internal::format_effect* ter_str_bind(std::string characters, ...);
internal::format_effect* furn_str_bind(std::string characters, ...);
internal::format_effect* simple_method_bind(std::string characters, ...);
internal::format_effect* end();

// Anything specified in here isn't finalized
namespace internal
{
 class determine_terrain;
 struct format_data
 {
  std::map<char, determine_terrain*> bindings;
 };

 // This class will become an interface in the future.
 class format_effect
 {
  private:
  std::string characters;
  std::vector<determine_terrain*> determiners;

  public:
   format_effect(std::string characters, std::vector<determine_terrain*> determiners);
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
        virtual int operator ()(map *, const int /*x*/, const int /*y*/) {
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
        virtual int operator ()(map *, const int /*x*/, const int /*y*/) {
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
   virtual int operator ()(map* m, const int x, const int y){return f(m,x,y);}
 };


} //END NAMESPACE mapf::internal

} //END NAMESPACE mapf

#endif // MAPGENFORMAT_H
