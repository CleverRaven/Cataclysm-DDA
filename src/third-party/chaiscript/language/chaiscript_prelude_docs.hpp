/// This file is not technically part of the ChaiScript API. It is used solely for generating Doxygen docs
/// regarding the ChaiScript standard runtime library.

/// \brief Items in this namespace exist in the ChaiScript language runtime. They are not part of the C++ API
namespace ChaiScript_Language
{

/// \page LangStandardLibraryRef ChaiScript Language Standard Library Reference
///
/// ChaiScript, at its core, has some very functional programming-inspired habits. Few places show this off as clearly 
/// as the prelude, itself a name taken as a nod to the popular functional language Haskell. This prelude is available 
/// to all standard ChaiScript applications, and provides a simple foundation for using numbers, strings, and ranges 
/// (the general category of Range cs and their iteration).
///


/// \brief Generic concept of a value in ChaiScript. 
///
/// The Object type exists merely as a concept. All objects in ChaiScript support this concept 
/// and have the following methods available to them. All objects are stored internally as chaiscript::Boxed_Value types.
///
/// \sa chaiscript::Boxed_Value
class Object
{
  public:
    /// \brief Returns the Type_Info value for this Object
    Type_Info get_type_info() const;

    /// \brief Returns true if the Object is of the named type
    bool is_type(string) const;

    /// \brief Returns true if the Object is of the Type_Info passed in
    bool is_type(Type_Info) const;

    /// \brief Returns true if the Object is immutable
    bool is_var_const() const;

    /// \brief Returns true if the Object is a pointer and the pointer is null
    bool is_var_null() const;

    /// \brief Returns true if the Object is stored as a pointer
    bool is_var_pointer() const;

    /// \brief Returns true if the Object is stored as a reference    
    bool is_var_reference() const;

    /// \brief Returns true if the Object does not contain a value is is undefined.
    bool is_var_undef() const;

    /// \brief Returns the registered name of the type of the object. 
    ///
    /// \sa Type_Info::name();
    string type_name() const;
};

/// \brief Item returned from a Range object from a Map
class Map_Pair
{
  public:
    /// \brief Returns the key of the Map entry
    const string first();

    /// \brief Returns the value Object of the Map entry 
    Object second();
};


/// \brief Maps strings to Objects
///
/// ChaiScript has a built in shortcut for generating Map objects:
/// 
/// Example:
/// \code
/// eval> var m = ["a":1, "b":2];
/// [<a,1>, <b,2>]
/// eval> m.count("a");
/// 1
/// eval> m.count("c");
/// 0
/// eval> m.size();
/// 2
/// \endcode
///
/// Implemented as std::map<Boxed_Value>
///
/// \sa Map_Pair
/// \sa chaiscript::bootstrap::standard_library::map_type
class Map
{
  public:
    /// \brief Returns an object that implements the Range concept for the Map_Pair's in this Map
    Range range();

    /// \brief Returns an object that implements the Const_Range concept for the Map_Pair's in this Map
    Const_Range range() const;

    /// \brief Returns the number of elements in the Map
    int size() const;

    /// \brief Returns the item at the given key, creating an undefined Object if the key does not yet exist in the map
    Object operator[](string);

    /// \brief Clears the map of all items
    void clear();

    /// \brief Returns the number of items in the Map with the given key. Returns 0 or 1 since this is not an std::multimap.
    int count(string) const;

    /// \brief Returns true if the map contains no items
    bool empty() const;

};


/// \brief A concept implemented by string, Vector and Map. It is convertible to Range, default constructable and back_insertable
class Container
{
  public:
    void push_back(Object);
    Range range();
    Const_Range range() const;
};


/// \brief Converts o into a string.
///
/// \code
/// eval> to_string(3).is_type("string") <br>
/// true<br>
/// \endcode
string to_string(Object o);


/// \brief Prints o to the terminal, without a trailing carriage return. Applies conversions to string automatically.
/// \code
/// eval> puts("hi, "); puts("there")
/// hi, thereeval>
/// \endcode
/// \sa to_string
/// \sa print
void puts(Object o);


/// \brief Prints o to the terminal, with a trailing carriage return. Applies conversions to string automatically
/// \code
/// eval> print("hello")
/// hello
/// eval>
/// \endcode
/// \sa to_string
/// \sa puts
void print(Object o);

/// \brief ChaiScript representation of std::string. It is an std::string but only some member are exposed to ChaiScript.
/// 
/// Because the ChaiScript string object is an std::string, it is directly convertible to and from std::string
/// using the chaiscript::boxed_cast and chaiscript::var functions.
///
/// With the exception of string::trim, string::rtrim, string::ltrim, all members are direct pass-throughs to the
/// std::string of the same name. 
///
/// \note Object and function notations are equivalent in ChaiScript. This means that
///       \c "bob".find("b") and \c find("bob", "b") are exactly the same. Most examples below follow the
///       second formation of the function calls.
/// \sa \ref keyworddef for extending existing C++ classes in ChaiScript
/// \sa chaiscript::bootstrap::standard_library::string_type
class string
{
  public:
    /// \brief Finds the first instance of substr.
    /// \code
    /// eval> find("abab", "ab")
    /// 0
    /// \endcode
    int find(string s) const;


    /// \brief Finds the last instance of substr.
    /// \code
    /// eval> rfind("abab", "ab")
    /// 2
    /// \endcode
    int rfind(string s) const;

    /// \brief Finds the first of characters in list in the string.
    ///
    /// \code
    /// eval> find_first_of("abab", "bec")
    /// 1
    /// \endcode
    int find_first_of(string list) const;

    /// \brief Finds the last of characters in list in the string.
    ///
    /// \code
    /// eval> find_last_of("abab", "bec")
    /// 3
    /// \endcode
    int find_last_of(string list) const;

    /// \brief Finds the first non-matching character to list in the str string.
    ///
    /// \code
    /// eval> find_first_not_of("abcd", "fec")
    /// 0
    /// \endcode
    int find_first_not_of(string list) const;

    /// \brief Finds the last non-matching character to list in the list string.
    ///
    /// \code
    /// eval> find_last_not_of("abcd", "fec")
    /// 3
    /// \endcode 
    int find_last_not_of(string list) const;

    /// \brief Removes whitespace from the front of the string, returning a new string
    ///
    /// \note This function is implemented as a ChaiScript function using the def member function notation.
    ///
    /// \code
    /// eval> ltrim("  bob")
    /// bob
    /// \endcode
    ///
    /// \sa \ref keyworddef
    string lstrim() const;

    /// \brief Removes whitespace from the back of the string, returning a new string
    ///
    /// \note This function is implemented as a ChaiScript function using the def member function notation.
    ///
    /// \code
    /// eval> rtrim("bob  ") + "|"
    /// bob|
    /// \endcode
    ///
    /// \sa \ref keyworddef
    string rtrim() const;

    /// \brief Removes whitespace from the front and back of the string, returning a new string
    ///
    /// \note This function is implemented as a ChaiScript function using the def member function notation.
    ///
    /// \code
    /// eval> trim("  bob  ") + "|"
    /// bob|
    /// \endcode
    /// 
    /// Equivalent to rtrim(ltrim("  bob  "));
    ///
    /// \sa \ref keyworddef
    string trim() const;

    /// \brief Returns the character at the given index in the string, const version
    const char &operator[](int t_index) const;

    /// \brief Returns the character at the given index in the string
    char &operator[](int t_index);

    /// \brief Returns underlying const char * for C api compatibility
    const char *c_str() const;

    /// \brief Returns a pointer to the raw data in the string
    const char *data() const;

    /// \brief Resets the string to empty
    void clear();

    /// \brief Returns true if the string is empty
    bool empty() const;

    /// \brief Returns the size of the string in bytes.
    ///
    /// This function normally returns size_t in C++. In ChaiScript the return value is cast to int
    /// for ease of use.
    int size() const;

    /// \brief Returns an object that implements the Range concept for the characters of this string
    Range range();

    /// \brief Returns an object that implements the Const_Range concept for the characters of this string
    Const_Range range() const;
};

/// \brief A concept in ChaiScript that is implemented by \ref string, Vector and Map. It provides
///        easy iteration over the elements in a container.
///
/// Implemented by the template chaiscript::bootstrap::standard_library::Bidir_Range
///
/// \sa Const_Range
class Range
{
  public:
    /// \brief Returns the last item of the range
    Object back();

    /// \brief Returns true if the front and back pointers have passed each other, if no items
    ///        are left in the Range
    bool empty() const;

    /// \brief Returns the first item of the range
    Object front();

    /// \brief Moves the back pointer back one.
    /// 
    /// \post back() returns the element at back() - 1;
    void pop_back();

    /// \brief Moves the front pointer forward one
    /// 
    /// \post front() returns the element at front() + 1;
    void pop_front();

};

/// \brief A concept in ChaiScript that is implemented by \ref string, Vector and Map. It provides
///        easy iteration over the elements in a container. Contained values are const.
///
/// Implemented by the template chaiscript::bootstrap::standard_library::Const_Bidir_Range
///
/// \sa Range
class Const_Range
{
  public:
    /// \brief Returns the last item of the range
    const Object back();

    /// \brief Returns true if the front and back pointers have passed each other, if no items
    ///        are left in the Range
    bool empty() const;

    /// \brief Returns the first item of the range
    const Object front();

    /// \brief Moves the back pointer back one.
    /// 
    /// \post back() returns the element at back() - 1;
    void pop_back();

    /// \brief Moves the front pointer forward one
    /// 
    /// \post front() returns the element at front() + 1;
    void pop_front();

};

/// \brief A vector of Objects
///
/// ChaiScript includes a shortcut for creating a Vector of Objects
/// 
/// Example:
/// \code
/// eval> var v = [1,2,3,4]
/// [1, 2, 3, 4]
/// eval> v[0];
/// 1
/// eval> v.size();
/// 4
/// \endcode 
///
/// Implemented with std::vector<chaiscript::Boxed_Value>
/// 
/// \sa chaiscript::bootstrap::standard_library::vector_type
class Vector
{
  public:
    /// \brief returns the Object at the given index. Throws an exception if the index does not exist
    Object operator[](int t_index);

    /// \brief returns a const Object at the given index. Throws an exception if the index does not exist.
    const Object operator[](int t_index) const;

    /// \brief returns the last item in the Vector
    Object back();

    /// \brief Clears the Vector of all items
    void clear();

    /// \brief Returns true if the Vector is contains 0 items
    bool empty();

    /// \brief Erases the element at the given index
    void erase_at(int t_index);

    /// \brief Returns the first item in the Vector
    Object front();

    /// \brief Inserts a new item in the Vector at the given index. The item is not cloned on insert
    /// 
    /// \sa insert_ref
    void insert_ref_at(int, Object);

    /// \brief Inserts a new item in the Vector at the given index. The item is cloned on insert
    /// 
    /// \sa insert_ref
    void insert_at(int, Object);
   
    /// \brief Removes the last item from the Vector
    void pop_back();

    /// \brief Adds an item to the end of the Vector. The item is not cloned.
    ///
    /// \sa push_back
    void push_back_ref(Object);

    /// \brief Adds an item to the end of the Vector. The item is cloned.
    ///
    /// \sa push_back_ref
    void push_back(Object);

    /// \brief Returns a Range object for the entire vector
    Range range();

    /// \brief Returns a Const_Range object for the entire vector
    Const_Range range() const;

    /// \brief Returns the number of elements in the Vector
    int size() const;

};

class Type_Info
{
  public:
    /// \brief Compares this Type_Info object with another one and returns true if the two types are the same
    ///        after const, pointer, reference are removed.
    bool bare_equal(Type_Info t_ti) const;

    /// \brief Returns the mangled C++ name for the type given by the compiler after const, pointer, reference is removed.
    string cpp_bare_name() const;

    /// \brief Returns the mangled C++ name for the type given by the compiler.
    string cpp_name() const;

    /// \brief Returns true if the type is const
    bool is_type_const() const;

    /// \brief Returns true if the type is a pointer
    bool is_type_pointer() const;

    /// \brief Returns true if the type is a reference 
    bool is_type_reference() const;

    /// \brief Returns true if the type is undefined 
    bool is_type_undef() const;

    /// \brief Returns true if the type is "void"
    bool is_type_void() const;

    /// \brief Returns the ChaiScript registered name for the type if one exists.
    string name() const;

};


/// \brief Represents a function object in ChaiScript
///
/// A function object may be one function, such as:
/// \code
/// var f = fun(x) { return x; }
/// \endcode
/// 
/// Or it may represent multiple functions
/// \code
/// var f2 = `-`; // represents the unary - as well as the set of binary - operators
/// \endcode
///
/// Guarded function example
/// \code
/// def f3(x) : x > 2 {
///  return x;
/// }
/// \endcode
///
/// Examples in the function definitions below will reference these examples
class Function
{
  public:
    /// \brief Returns the annotation description of the function
    string get_annotation() const;

    /// \brief Returns the arity of the function, -1 if the function takes a variable number of parameters
    ///
    /// Example:
    /// \code
    /// eval> f.get_arity()
    /// 1
    /// eval> f2.get_arity()
    /// -1
    /// \endcode
    int get_arity() const;

    /// \brief Returns a vector of the contained functions
    /// 
    /// Example:
    /// \code
    /// eval> f.get_contained_functions().size()
    /// 0
    /// eval> f2.get_contained_functions().size()
    /// 11
    /// eval> var v = f2.get_contained_functions();
    /// v[0].get_arity()
    /// 2
    /// \endcode
    Vector get_contained_functions() const;

    /// \brief Returns a function guard as function
    /// 
    /// Example:
    /// \code
    /// eval> f.get_guard() // Throws exception
    /// Function does not have a guard
    /// eval> f3.get_guard().get_arity()
    /// 1
    /// \endcode
    Function get_guard() const;

    /// \brief Returns a vector of Type_Info objects that represent the param types for this function.
    ///        The first value in the list is the return type.
    /// 
    /// If this function is a conglomerate of several functions (get_contained_values().size() > 0)
    /// then the function returns as many Type_Info objects as it can. If the functions contained all have
    /// the same arity, then it represents the arity. If they have different arities, it returns only
    /// one value - the return type. 
    /// 
    /// For each parameter that is the same type, the type is returned. If the types are different
    /// then a Type_Info for Object is returned.
    ///
    /// Example:
    /// \code
    /// eval> f2.get_param_types().size(); // Returns a Type_Info for Object for the return type
    /// 1
    /// \endcode
    Vector get_param_types() const;

    /// \brief Returns true if the function has a guard to it. Always returns false for a conglomerate function
    bool has_guard() const;

    /// \brief Calls the function with the given set of parameters and returns the value;
    /// 
    /// Example:
    /// \code
    /// eval> `-`.call([2,1]);
    /// 1
    /// \endcode
    Object call(Vector t_params) const;
}



/// \brief Returns the max of a or b. Requires that operator>(a, b) exists
/// Equivalent to 
/// \code
/// return a>b?a:b;
/// \endcode
///
/// Example:
/// \code
/// eval> max(4, 10)
/// 10
/// \endcode
Object max(Object a, Object b);

/// \brief Returns the min of a or b. Requires that operator<(a, b) exists
///
/// Equivalent to 
/// \code
/// return a<b?a:b;
/// \endcode
///
/// Example:
/// \code
/// eval> min(4, 10)
/// 4
/// \endcode
Object min(Object a, Object b);

/// \brief Returns true if x is an even integer. 
/// 
/// Will also work on any non-integer type for which an operator%(x, int) exists
///
/// Example:
/// \code
/// eval> even(4)
/// true
/// \endcode
bool even(Object x);

/// \brief Returns true if x is an odd integer. 
/// 
/// Will also work on any non-integer type for which an operator%(x, int) exists
///
/// Example:
/// \code
/// eval> odd(4)
/// false 
/// \endcode
bool even(Object x);


/// \brief Applies the function f over each element in the Range c.
///
/// Example:
/// \code
/// eval> for_each([1, 2, 3], print)
/// 1
/// 2
/// 3
/// \endcode
void for_each(Range c, Function f);


/// \brief Applies f over each element in the Range c, joining all the results.
///
/// Example:
/// \code
/// eval> map([1, 2, 3], odd)
/// [true, false, true]
/// \endcode
Object map(Range c, Function f);


/// \brief Starts with the initial value and applies the function f to it and the first element of the Range c. 
///        The result is then applied to the second element, and so on until the elements are exhausted.
///
/// Example:
/// \code
/// eval> foldl([1, 2, 3, 4], `+`, 0)
/// 10
/// \endcode
Object foldl(Range c, Function f, Object initial);


/// \brief Returns the sum total of the values in the Range c.
///
/// Example:
/// \code
/// eval> sum([1, 2, 3, 4])
/// 10
/// \endcode
///
/// Equivalent to:
/// \code
/// foldl(c, `+`, 0.0);
/// \endcode
Numeric sum(Range c);


/// \brief Returns the product of the value in the Range c.
///
/// Example:
/// \code
/// eval> product([1, 2, 3, 4])
/// 24
/// \endcode
///
/// Equivalent to:
/// \code
/// foldl(c, `*`, 1.0);
/// \endcode
Numeric product(Range c);


/// \brief Takes num elements from the Range c, returning them.
///
/// Example:
/// \code
/// eval> take([1, 2, 3, 4], 2)
/// [1, 2]
/// \endcode
///
/// \returns A container of the same type that was passed in
Object take(Range c, int num);


/// \brief Takes elements from the Range c that match function f, stopping at the first non-match, returning them as a new Vector.
///
/// Example:
/// \code
/// eval> take_while([1, 2, 3], odd)
/// [1]
/// \endcode
///
/// \returns A container of the same type that was passed in
Object take_while(Range c, Function f);


/// \brief Drops num elements from the Range c, returning the remainder.
///
/// Example:
/// \code
/// eval> drop([1, 2, 3, 4], 2)
/// [3, 4]
/// \endcode
///
/// \returns A container of the same type that was passed in
Object drop(Range c, int num);


/// \brief Drops elements from the Range c that match f, stopping at the first non-match, returning the remainder.
///
/// Example:
/// \code
/// eval> drop_while([1, 2, 3], odd)         
/// [2, 3]
/// \endcode
Object drop_while(Range c, Function f);


/// \brief Similar to foldl, this takes the first two elements as its starting values for f. This assumes Range c has at least 2 elements.
///
/// Example:
/// \code
/// eval> reduce([1, 2, 3, 4], `+`)
/// 10
/// \endcode
Object reduce(Range c, Function f);


/// \brief Takes elements from Container c that match function f, return them.
///
/// Example:
/// \code
/// eval> filter([1, 2, 3, 4], odd)
/// [1, 3]
/// \endcode
Object filter(Container c, Function f);


/// \brief Joins the elements of the Range c into a string, delimiting each with the delim string.
///
/// Example:
/// \code
/// eval> join([1, 2, 3], "*") 
/// 1*2*3
/// \endcode
string join(Range c, string delim);


/// \brief Returns the contents of the Container c in reversed order.
///
/// Example:
/// \code
/// eval> reverse([1, 2, 3, 4, 5, 6, 7])
/// [7, 6, 5, 4, 3, 2, 1]
/// \endcode
Container reverse(Container c);


/// \brief Generates a new Vector filled with values starting at x and ending with y.
///
/// Works on types supporting operator<=(x, y) and operator++(x)
///
/// Example:
/// \code
/// eval> generate_range(1, 10)
/// [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
/// \endcode
Vector generate_range(Object x, Object y);


/// \brief Returns a new Range with x and y concatenated.
///
/// Example:
/// \code
/// eval> concat([1, 2, 3], [4, 5, 6])
/// [1, 2, 3, 4, 5, 6]
/// \endcode
Object concat(Range x, Range y);


/// \brief Returns a new Vector with x and y as its values.
///
/// Example:
/// \code
/// eval> collate(1, 2)
/// [1, 2]
/// \endcode
Vector collate(Object x, Object y);


/// \brief Applies f to elements of x and y, returning a new Vector with the result of each application.
///
/// Example:
/// \code
/// eval> zip_with(`+`, [1, 2, 3], [4, 5, 6])
/// [5, 7, 9]
/// \endcode
Vector zip_with(Function f, Range x, Range y);


/// \brief Collates elements of x and y, returning a new Vector with the result.
///
/// Example:
/// \code
/// eval> zip([1, 2, 3], [4, 5, 6])
/// [[1, 4], [2, 5], [3, 6]]
/// \endcode
Vector zip(Range x, Range y);


/// \brief returns true if there exists a call to the Function f that takes the given parameters
///
/// Example:
/// \code
/// eval> call_exists(`+`, 1, 2)
/// true
/// \endcode
bool call_exists(Function f, ...);

/// \brief Reverses a Range object so that the elements are accessed in reverse
Range retro(Range);

/// \brief Reverses a Const_Range object so that the elements are accessed in reverse
Const_Range retro(Const_Range);


/// \brief Raises the given object as an exception. Any type of object can be thrown.
///
/// Example:
/// \code
/// eval> try { throw(1); } catch (e) { print("Exception caught: " + to_string(e)); }
/// Exception caught: 1
/// \endcode
/// 
/// \sa \ref keywordtry
void throw(Object); 
}

