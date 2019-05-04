#pragma once
#ifndef NAME_H
#define NAME_H

#include <string>
#include <map>
#include <vector>


enum NameFlags
{
    NAME_IS_MALE_NAME,
    NAME_IS_FEMALE_NAME,
    NAME_IS_UNISEX_NAME,
    NAME_IS_GIVEN_NAME,
    NAME_IS_FAMILY_NAME,
    NAME_IS_NICK_NAME,
    NAME_IS_TOWN_NAME,
    NAME_IS_WORLD_NAME
};

class RandomName
{

private:

    std::vector<std::string> namesMale;
    std::vector<std::string> namesFemale;
    std::vector<std::string> namesUnisex;
    std::vector<std::string> namesGivenMale;
    std::vector<std::string> namesGivenFemale;
    std::vector<std::string> namesFamily;
    std::vector<std::string> namesNick;
    std::vector<std::string> namesTown;
    std::vector<std::string> namesWorld;

    /*
     * Here will be the instance stored.
     */
    static RandomName *instance;

    /*
     * Private constructor to prevent instancing.
     */
    RandomName();

    /*
     * Private constructor to prevent instancing.
     */
    RandomName( std::string pathToFile );

    void loadNames( std::string pathToFile );

public:

    /*
     * Static access method.
     */
    static RandomName *getInstance();

    static RandomName *getInstance(std::string pathToFile);

    std::string getRandomName( NameFlags flag );

    void toString();
};

#endif
