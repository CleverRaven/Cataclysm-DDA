#include "name.h"

#include <cstddef>
#include <utility>

#include "cata_utility.h"
#include "json.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "path_info.h"

/*
 * Nullptr, because instance will be initialized on demand.
 */
RandomName* RandomName::instance = nullptr;

RandomName::RandomName( )
{
    loadNames("");
}

RandomName::RandomName( std::string pathToFile )
{
    loadNames( pathToFile );
}

void RandomName::loadNames( std::string pathToFile )
{
    std::string pathToFileJSON = pathToFile;

    if (pathToFileJSON.empty())
    {
        Path *path = Path::getInstance();

        pathToFileJSON = path->getPathForValueKey("NAMES_FILE");
    }

    std::ifstream fileJSON;

    fileJSON.open(pathToFileJSON, std::ifstream::in);

    JsonIn *inputJSON = new JsonIn(fileJSON);

    inputJSON->start_array();

    while(!inputJSON->end_array())
    {
        JsonObject objectJSON = inputJSON->get_object();

        if (objectJSON.get_string("usage") == "nick")
        {
            namesNick.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "backer")
        {
            if (objectJSON.get_string("gender") == "male")
            {
                namesMale.push_back(objectJSON.get_string("name"));
            }
            else if (objectJSON.get_string("gender") == "female")
            {
                namesFemale.push_back(objectJSON.get_string("name"));
            }
            else
            {
                namesUnisex.push_back(objectJSON.get_string("name"));
            }
        }
        else if (objectJSON.get_string("usage") == "city")
        {
            namesTown.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "family")
        {
            namesFamily.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "given")
        {
            if (objectJSON.get_string("gender") == "male")
            {
                namesGivenMale.push_back(objectJSON.get_string("name"));
            }
            else
            {
                namesGivenFemale.push_back(objectJSON.get_string("name"));
            }
        }
        else if (objectJSON.get_string("usage") == "world")
        {
            namesWorld.push_back(objectJSON.get_string("name"));
        }
    }

    fileJSON.close();
}

RandomName* RandomName::getInstance( )
{
    if (instance == nullptr)
    {
        instance = new RandomName();
    }

    return instance;
}

RandomName* RandomName::getInstance( std::string pathToFile )
{
    if (instance == nullptr)
    {
        instance = new RandomName(pathToFile);
    }

    return instance;
}

std::string RandomName::getRandomName( NameFlags flag )
{
    //TODO: Generate Number Random

    if (flag == NAME_IS_MALE_NAME)
    {
        short choice = rng(0, namesMale.size() - 1);
        return namesMale[choice];
    }
    else if (flag == NAME_IS_FEMALE_NAME)
    {
        short choice = rng(0, namesFemale.size() - 1);
        return namesFemale[choice];
    }
    else if (flag == NAME_IS_UNISEX_NAME)
    {
        short choice = rng(0, namesUnisex.size() - 1);
        return namesUnisex[choice];
    }
    else if (flag == NAME_IS_GIVEN_NAME)
    {
        short choiceGiven = rng(0, 1);

        if (choiceGiven == 1)
        {
            short choice = rng(0, namesGivenMale.size() - 1);
            return namesGivenMale[choice];
        }
        else
        {
            short choice = rng(0, namesGivenFemale.size() - 1);
            return namesGivenFemale[choice];
        }

    }
    else if (flag == NAME_IS_FAMILY_NAME)
    {
        short choice = rng(0, namesFamily.size() - 1);
        return namesFamily[choice];
    }
    else if (flag == NAME_IS_NICK_NAME)
    {
        short choice = rng(0, namesNick.size() - 1);
        return namesNick[choice];
    }
    else if (flag == NAME_IS_TOWN_NAME)
    {
        short choice = rng(0, namesTown.size() - 1);
        return namesTown[choice];
    }
    else if (flag == NAME_IS_WORLD_NAME)
    {
        short choice = rng(0, namesWorld.size() - 1);
        return namesWorld[choice];
    }
}

void RandomName::toString( )
{
    std::cout << std::endl << namesMale.size() << std::endl;
    std::cout << std::endl << namesFemale.size() << std::endl;
    std::cout << std::endl << namesUnisex.size() << std::endl;
    std::cout << std::endl << namesGivenMale.size() << std::endl;
    std::cout << std::endl << namesGivenFemale.size() << std::endl;
    std::cout << std::endl << namesFamily.size() << std::endl;
    std::cout << std::endl << namesNick.size() << std::endl;
    std::cout << std::endl << namesWorld.size() << std::endl;
}
