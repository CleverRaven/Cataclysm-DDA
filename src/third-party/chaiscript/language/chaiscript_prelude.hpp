// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// and 2009-2017, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_PRELUDE_HPP_
#define CHAISCRIPT_PRELUDE_HPP_

namespace chaiscript {
struct ChaiScript_Prelude { 
  static std::string chaiscript_prelude() {  return R"chaiscript(

def lt(l, r) { 
  if (call_exists(`<`, l, r)) { 
    l < r 
  } else { 
    type_name(l) < type_name(r) 
  } 
}


def gt(l, r) { 
  if (call_exists(`>`, l, r)) { 
    l > r 
  } else { 
    type_name(l) > type_name(r) 
  } 
}

def eq(l, r) { 
  if (call_exists(`==`, l, r)) { 
    l == r 
  } else { 
    false 
  } 
}

def new(x) { 
  eval(type_name(x))(); 
}

def clone(double x) {
  double(x).clone_var_attrs(x)
}

def clone(string x) {
  string(x).clone_var_attrs(x)
}

def clone(vector x) {
  vector(x).clone_var_attrs(x)
}


def clone(int x) {
  int(x).clone_var_attrs(x)
}

def clone(x) : function_exists(type_name(x)) && call_exists(eval(type_name(x)), x)
{
  eval(type_name(x))(x).clone_var_attrs(x);
}


# to_string for Pair()
def to_string(x) : call_exists(first, x) && call_exists(second, x) { 
  "<" + x.first.to_string() + ", " + x.second.to_string() + ">"; 
}

# to_string for containers
def to_string(x) : call_exists(range, x) && !x.is_type("string"){ 
  "[" + x.join(", ") + "]"; 
}

# Prints to console with no carriage return
def puts(x) { 
  print_string(x.to_string()); 
}

# Prints to console with carriage return
def print(x) { 
  println_string(x.to_string()); 
}

# Returns the maximum value of two numbers
def max(a, b) {
  if (a>b) { 
    a 
  } else { 
    b 
  } 
} 

# Returns the minimum value of two numbers
def min(a, b) 
{ 
  if (a<b) 
  { 
    a 
  } else { 
    b 
  } 
}


# Returns true if the value is odd
def odd(x)  { 
  if (x % 2 == 1) 
  { 
    true 
  } else { 
    false 
  } 
}


# Returns true if the value is even
def even(x) 
{ 
  if (x % 2 == 0) 
  { 
    true 
  } else { 
    false 
  } 
} 


# Inserts the third value at the position of the second value into the container of the first
# while making a clone. 
def insert_at(container, pos, x) 
{ 
  container.insert_ref_at(pos, clone(x)); 
} 

# Returns the reverse of the given container
def reverse(container) {
  auto retval := new(container); 
  auto r := range(container); 
  while (!r.empty()) { 
    retval.push_back(r.back()); 
    r.pop_back(); 
  } 
  retval; 
}


def range(r) : call_exists(range_internal, r)
{
  var ri := range_internal(r);
  ri.get_var_attr("internal_obj") := r;
  ri;
}

# Return a range from a range 
def range(r) : call_exists(empty, r) && call_exists(pop_front, r) && call_exists(pop_back, r) && call_exists(back, r) && call_exists(front, r) 
{
  clone(r);
}


# The retro attribute that contains the underlying range 
attr retro::m_range; 

# Creates a retro from a retro by returning the original range
def retro(r) : call_exists(get_type_name, r) && get_type_name(r) == "retro" 
{ 
  clone(r.m_range) 
}


# Creates a retro range from a range
def retro::retro(r) : call_exists(empty, r) && call_exists(pop_front, r) && call_exists(pop_back, r) && call_exists(back, r) && call_exists(front, r) 
{ 
  this.m_range = r; 
}

# Returns the first value of a retro
def retro::front()  
{ 
  back(this.m_range) 
}

# Returns the last value of a retro
def retro::back()  
{ 
  front(this.m_range) 
}

# Moves the back iterator of a retro towards the front by one 
def retro::pop_back()  
{
  pop_front(this.m_range) 
}

# Moves the front iterator of a retro towards the back by one 
def retro::pop_front()  
{ 
  pop_back(this.m_range) 
} 

# returns true if the retro is out of elements 
def retro::empty() 
{ 
  empty(this.m_range); 
} 

# Performs the second value function over the container first value
def for_each(container, func) : call_exists(range, container) { 
  var t_range := range(container); 
  while (!t_range.empty()) { 
    func(t_range.front()); 
    t_range.pop_front(); 
  } 
} 

def any_of(container, func) : call_exists(range, container) { 
  var t_range := range(container); 
  while (!t_range.empty()) { 
    if (func(t_range.front())) {
      return true;
    }
    t_range.pop_front(); 
  }
  false;
}

def all_of(container, func) : call_exists(range, container) { 
  var t_range := range(container); 
  while (!t_range.empty()) { 
    if (!func(t_range.front())) {
      return false;
    }
    t_range.pop_front(); 
  }

  true;
}

def back_inserter(container) {
  bind(push_back, container, _);
}

def contains(container, item, compare_func) : call_exists(range, container) { 
  auto t_range := range(container); 
  while (!t_range.empty()) { 
    if ( compare_func(t_range.front(), item) ) { 
      return true; 
    } 

    t_range.pop_front(); 
  } 
  false; 
} 

def contains(container, item) { 
  contains(container, item, eq) 
} 

def map(container, func, inserter) : call_exists(range, container) { 
  auto range := range(container); 
  while (!range.empty()) { 
    inserter(func(range.front())); 
    range.pop_front(); 
  } 
} 

# Performs the second value function over the container first value. Creates a new container with the results
def map(container, func) { 
  auto retval := new(container); 
  map(container, func, back_inserter(retval));
  retval;
}

# Performs the second value function over the container first value. Starts with initial and continues with each element.
def foldl(container, func, initial) : call_exists(range, container){ 
  auto retval = initial; 
  auto range := range(container); 
  while (!range.empty()) { 
    retval = (func(range.front(), retval)); 
    range.pop_front(); 
  } 
  retval; 
}

# Returns the sum of the elements of the given value
def sum(container) { 
  foldl(container, `+`, 0.0) 
} 

# Returns the product of the elements of the given value
def product(container) { 
  foldl(container, `*`, 1.0) 
} 

# Returns a new container with the elements of the first value concatenated with the elements of the second value
def concat(x, y) : call_exists(clone, x) { 
  auto retval = x; 
  auto inserter := back_inserter(retval); 
  auto range := range(y); 
  while (!range.empty()) { 
    inserter(range.front()); 
    range.pop_front(); 
  } 
  retval; 
}


def take(container, num, inserter) : call_exists(range, container) { 
  auto r := range(container); 
  auto i = num; 
  while ((i > 0) && (!r.empty())) { 
    inserter(r.front()); 
    r.pop_front(); 
    --i; 
  } 
}


# Returns a new container with the given number of elements taken from the container
def take(container, num) {
  auto retval := new(container); 
  take(container, num, back_inserter(retval)); 
  retval; 
}


def take_while(container, f, inserter) : call_exists(range, container) { 
  auto r := range(container); 
  while ((!r.empty()) && f(r.front())) { 
    inserter(r.front()); 
    r.pop_front(); 
  } 
} 


# Returns a new container with the given elements match the second value function
def take_while(container, f) {
  auto retval := new(container); 
  take_while(container, f, back_inserter(retval)); 
  retval;
}


def drop(container, num, inserter) : call_exists(range, container) { 
  auto r := range(container); 
  auto i = num; 
  while ((i > 0) && (!r.empty())) { 
    r.pop_front(); 
    --i; 
  } 
  while (!r.empty()) { 
    inserter(r.front()); 
    r.pop_front(); 
  } 
}


# Returns a new container with the given number of elements dropped from the given container 
def drop(container, num) {
  auto retval := new(container); 
  drop(container, num, back_inserter(retval)); 
  retval; 
}


def drop_while(container, f, inserter) : call_exists(range, container) { 
  auto r := range(container); 
  while ((!r.empty())&& f(r.front())) { 
    r.pop_front(); 
  } 
  while (!r.empty()) { 
    inserter(r.front()); 
    r.pop_front(); 
  } 
}


# Returns a new container with the given elements dropped that match the second value function
def drop_while(container, f) {
  auto retval := new(container); 
  drop_while(container, f, back_inserter(retval)); 
  retval; 
}


# Applies the second value function to the container. Starts with the first two elements. Expects at least 2 elements.
def reduce(container, func) : container.size() >= 2 && call_exists(range, container) { 
  auto r := range(container); 
  auto retval = r.front(); 
  r.pop_front(); 
  retval = func(retval, r.front()); 
  r.pop_front(); 
  while (!r.empty()) { 
    retval = func(retval, r.front()); 
    r.pop_front(); 
  } 
  retval; 
}


# Returns a string of the elements in container delimited by the second value string
def join(container, delim) { 
  auto retval = ""; 
  auto range := range(container); 
  if (!range.empty()) { 
    retval += to_string(range.front()); 
    range.pop_front(); 
    while (!range.empty()) { 
      retval += delim; 
      retval += to_string(range.front()); 
      range.pop_front(); 
    } 
  } 
  retval; 
}


def filter(container, f, inserter) : call_exists(range, container) { 
  auto r := range(container); 
  while (!r.empty()) { 
    if (f(r.front())) { 
      inserter(r.front()); 
    } 
    r.pop_front(); 
  } 
} 


# Returns a new Vector which match the second value function
def filter(container, f) { 
  auto retval := new(container); 
  filter(container, f, back_inserter(retval));
  retval;
}


def generate_range(x, y, inserter) { 
  auto i = x; 
  while (i <= y) { 
    inserter(i); 
    ++i; 
  } 
}


# Returns a new Vector which represents the range from the first value to the second value
def generate_range(x, y) { 
  auto retval := Vector(); 
  generate_range(x,y,back_inserter(retval)); 
  retval; 
}


# Returns a new Vector with the first value to the second value as its elements
def collate(x, y) { 
  return [x, y]; 
}


def zip_with(f, x, y, inserter) : call_exists(range, x) && call_exists(range, y) { 
  auto r_x := range(x); 
  auto r_y := range(y); 
  while (!r_x.empty() && !r_y.empty()) { 
    inserter(f(r_x.front(), r_y.front())); 
    r_x.pop_front(); 
    r_y.pop_front(); 
  } 
}


# Returns a new Vector which joins matching elements of the second and third value with the first value function
def zip_with(f, x, y) { 
  auto retval := Vector(); 
  zip_with(f,x,y,back_inserter(retval)); 
  retval;
}


# Returns a new Vector which joins matching elements of the first and second
def zip(x, y) { 
  zip_with(collate, x, y); 
}


# Returns the position of the second value string in the first value string
def string::find(string substr) { 
  find(this, substr, size_t(0)); 
}


# Returns the position of last match of the second value string in the first value string
def string::rfind(string substr) { 
  rfind(this, substr, size_t(-1)); 
}


# Returns the position of the first match of elements in the second value string in the first value string
def string::find_first_of(string list) { 
  find_first_of(this, list, size_t(0)); 
} 


# Returns the position of the last match of elements in the second value string in the first value string
def string::find_last_of(string list) {
  find_last_of(this, list, size_t(-1)); 
} 


# Returns the position of the first non-matching element in the second value string in the first value string
def string::find_first_not_of(string list) { 
  find_first_not_of(this, list, size_t(0)); 
} 


# Returns the position of the last non-matching element in the second value string in the first value string
def string::find_last_not_of(string list) { 
  find_last_not_of(this, list, size_t(-1)); 
} 


def string::ltrim() { 
  drop_while(this, fun(x) { x == ' ' || x == '\t' || x == '\r' || x == '\n'}); 
} 


def string::rtrim() { 
  reverse(drop_while(reverse(this), fun(x) { x == ' ' || x == '\t' || x == '\r' || x == '\n'})); 
} 


def string::trim() { 
  ltrim(rtrim(this)); 
}


def find(container, value, Function compare_func) : call_exists(range, container) { 
  auto range := range(container); 
  while (!range.empty()) { 
    if (compare_func(range.front(), value)) { 
      return range; 
    } else { 
      range.pop_front(); 
    } 
  } 
  range; 
} 


def find(container, value) { 
  find(container, value, eq) 
} 


)chaiscript";
}

};
}

#endif /* CHAISCRIPT_PRELUDE_HPP_ */
