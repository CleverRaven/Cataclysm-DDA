#ifndef ITEM_SEARCH_H
#define ITEM_SEARCH_H
#include <string>
#include <functional>
class item;
/**
 * Get a function that returns true if the item matches the query.
 */
std::function<bool(const item&)> item_filter_from_string(std::string);
#endif
