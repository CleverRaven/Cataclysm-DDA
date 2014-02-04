#ifndef _BASECAMP_H_
#define _BASECAMP_H_

#include <string>

class basecamp
{
public:
    basecamp();
    basecamp(std::string const& name_, int const posx_, int const posy_);

    inline bool is_valid() const { return !name.empty(); }
    inline int board_x() const { return posx; }
    inline int board_y() const { return posy; }
    inline std::string const& camp_name() const { return name; }

    std::string board_name() const;

    // Save/load
    std::string save_data() const;
    void load_data(std::string const& data);

private:
    std::string name;
    int posx, posy; // location of associated bulletin board
};


#endif
