#include "itype.h"
#include "ammo.h"
#include "game.h"
#include "monstergenerator.h"
#include "item_factory.h"
#include <fstream>
#include <stdexcept>

// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() const
{
    return !use_methods.empty();
}

bool itype::can_use( std::string iuse_name ) const
{
    const use_function *func;

    try {
        func = item_controller->get_iuse( iuse_name );
    } catch (const std::out_of_range &e) {
        debugmsg("itype::can_use attempted to test for invalid iuse function %s", iuse_name.c_str());
        return false;
    }

    return std::find( use_methods.cbegin(), use_methods.cend(),
                      *func ) != use_methods.cend();
}

bool itype::is_covering(body_part bp) const
{
    if (!is_armor()) {
        return false;
    }
    const it_armor *armor = dynamic_cast<const it_armor *>(this);
    return armor->covers.test(bp);
}


bool itype::is_sided(body_part bp) const
{
    if (!is_armor()) {
        return false;
    }
    const it_armor *armor = dynamic_cast<const it_armor *>(this);
    return armor->sided.test(bp);
}

int itype::invoke( player *p, item *it, bool active, point pos )
{
    int charges_to_use = 0;
    for( auto method = use_methods.begin();
         charges_to_use >= 0 && method != use_methods.end(); ++method ) {
        charges_to_use = method->call( p, it, active, pos );
    }
    return charges_to_use;
}


void Item_factory::init_old()
{
    auto &itypes = m_templates; // So I don't have to change that in the code below

    // First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
    add_item_type(
        new itype("null", 0, "none", "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0) );
    itypes["null"]->item_tags.insert( "PSEUDO" );
    // Corpse - a special item
    add_item_type(
        new itype("corpse", 0, "corpse", "corpses", _("A dead body."), '%', c_white, "null", "null", PNULL,
                  0, 0,
                  0, 0, 1) );
    itypes["corpse"]->item_tags.insert("NO_UNLOAD");
    itypes["corpse"]->item_tags.insert( "PSEUDO" );
    // Fire - only appears in crafting recipes
    add_item_type(
        new itype("fire", 0, "nearby fire", "none",
                  "Some fire - if you are reading this it's a bug! (itypdef:fire)",
                  '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0) );
    itypes["fire"]->item_tags.insert( "PSEUDO" );
    // Integrated toolset - ditto
    add_item_type(
        new itype("toolset", 0, "integrated toolset", "none",
                  "A fake item. If you are reading this it's a bug! (itypdef:toolset)",
                  '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0) );
    itypes["toolset"]->item_tags.insert( "PSEUDO" );
    // For smoking drugs
    add_item_type(
        new itype("apparatus", 0, "a smoking device and a source of flame", "none",
                  "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
                  '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0) );
    itypes["apparatus"]->item_tags.insert( "PSEUDO" );
    // For CVD Forging
    add_item_type(
        new itype("cvd_machine", 0, "cvd machine", "none",
                  "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
                  '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0) );
    itypes["cvd_machine"]->item_tags.insert( "PSEUDO" );

    // SOFTWARE
#define SOFTWARE(id, name, name_plural, price, swtype, power, description) \
    add_item_type( new it_software(id, price, name, name_plural, description,\
                               ' ', c_white, "null", "null", 0, 0, 0, 0, 0, swtype, power) )

    //Macguffins
#define MACGUFFIN(id, name, name_plural, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
    add_item_type( new it_macguffin(id, price, name, name_plural, description,\
                            sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, readable,\
                            function) )

    SOFTWARE("software_useless", "misc software", "none", 300, SW_USELESS, 0, _("\
A miscellaneous piece of hobby software. Probably useless."));

    SOFTWARE("software_hacking", "hackPRO", "none", 800, SW_HACKING, 2, _("\
A piece of hacking software."));

    SOFTWARE("software_medical", "MediSoft", "none", 600, SW_MEDICAL, 2, _("\
A piece of medical software."));

    SOFTWARE("software_math", "MatheMAX", "none", 500, SW_SCIENCE, 3, _("\
A piece of mathematical software."));

    SOFTWARE("software_blood_data", "infection data", "none", 200, SW_DATA, 5, _("\
Medical data on zombie blood."));

    MACGUFFIN("note", "note", "notes", 0, '?', c_white, "paper", "null", 1, 3, 0, 0, 0,
              true, &iuse::mcg_note, _("\
A hand-written paper note."));

    itype *m_missing_item = new itype();
    // intentionally left untranslated
    // because using _() at global scope is problematic,
    // and if this appears it's a bug anyway.
    m_missing_item->name = "Error: Item Missing.";
    m_missing_item->name_plural = "Error: Item Missing.";
    m_missing_item->description =
        "There is only the space where an object should be, but isn't. No item template of this type exists.";
    add_item_type( m_missing_item );
}

std::string ammo_name(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->name();
}

itype_id default_ammo(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->default_ammotype();
}
