-----------------------------------------------------------------------------
-- JSON4Lua: JSON encoding / decoding support for the Lua language.
-- json Module.
-- Author: Craig Mason-Jones
-- Homepage: http://json.luaforge.net/
-- Version: 0.9.50
-- This module is released under the MIT License (MIT).
-- Please see LICENCE.txt for details.
--
-- USAGE:
-- This module exposes two functions:
--   encode(o)
--     Returns the table / string / boolean / number / nil / json.null value as a JSON-encoded string.
--   decode(json_string)
--     Returns a Lua object populated with the data encoded in the JSON string json_string.
--
-- REQUIREMENTS:
--   compat-5.1 if using Lua 5.0
--
-- CHANGELOG
--	 0.9.50 Radical performance improvement on decode from Eike Decker. Many thanks!
--	 0.9.40 Changed licence to MIT License (MIT)
--   0.9.20 Introduction of local Lua functions for private functions (removed _ function prefix). 
--          Fixed Lua 5.1 compatibility issues.
--   		Introduced json.null to have null values in associative arrays.
--          encode() performance improvement (more than 50%) through table.concat rather than ..
--          Introduced decode ability to ignore /**/ comments in the JSON string.
--   0.9.10 Fix to array encoding / decoding to correctly manage nil/null values in arrays.
-----------------------------------------------------------------------------

-----------------------------------------------------------------------------
-- Imports and dependencies
-----------------------------------------------------------------------------
local math = require('math')
local string = require("string")
local table = require("table")
local tostring = tostring

local base = _G

-----------------------------------------------------------------------------
-- Module declaration
-----------------------------------------------------------------------------
module("json")

-- Public functions

-- Private functions
local decode_scanArray
local decode_scanComment
local decode_scanConstant
local decode_scanNumber
local decode_scanObject
local decode_scanString
local decode_scanWhitespace
local encodeString
local isArray
local isEncodable

-----------------------------------------------------------------------------
-- PUBLIC FUNCTIONS
-----------------------------------------------------------------------------
--- Encodes an arbitrary Lua object / variable.
-- @param v The Lua object / variable to be JSON encoded.
-- @return String containing the JSON encoding in internal Lua string format (i.e. not unicode)
function encode (v)
  -- Handle nil values
  if v==nil then
    return "null"
  end
  
  local vtype = base.type(v)  

  -- Handle strings
  if vtype=='string' then    
    return '"' .. encodeString(v) .. '"'	    -- Need to handle encoding in string
  end
  
  -- Handle booleans
  if vtype=='number' or vtype=='boolean' then
    return base.tostring(v)
  end
  
  -- Handle tables
  if vtype=='table' then
    local rval = {}
    -- Consider arrays separately
    local bArray, maxCount = isArray(v)
    if bArray then
      for i = 1,maxCount do
        table.insert(rval, encode(v[i]))
      end
    else	-- An object, not an array
      for i,j in base.pairs(v) do
        if isEncodable(i) and isEncodable(j) then
          table.insert(rval, '"' .. encodeString(i) .. '":' .. encode(j))
        end
      end
    end
    if bArray then
      return '[' .. table.concat(rval,',') ..']'
    else
      return '{' .. table.concat(rval,',') .. '}'
    end
  end
  
  -- Handle null values
  if vtype=='function' and v==null then
    return 'null'
  end
  
  base.assert(false,'encode attempt to encode unsupported type ' .. vtype .. ':' .. base.tostring(v))
end


--- Decodes a JSON string and returns the decoded value as a Lua data structure / value.
-- @param s The string to scan.
-- @return Lua objectthat was scanned, as a Lua table / string / number / boolean or nil.
function decode(s)
	-- Function is re-defined below after token and other items are created.
	-- Just defined here for code neatness.
	return null
end

--- The null function allows one to specify a null value in an associative array (which is otherwise
-- discarded if you set the value with 'nil' in Lua. Simply set t = { first=json.null }
function null()
  return null -- so json.null() will also return null ;-)
end

-----------------------------------------------------------------------------
-- Internal, PRIVATE functions.
-----------------------------------------------------------------------------

--- Encodes a string to be JSON-compatible.
-- This just involves back-quoting inverted commas, back-quotes and newlines, I think ;-)
-- @param s The string to return as a JSON encoded (i.e. backquoted string)
-- @return The string appropriately escaped.
local qrep = {["\\"]="\\\\", ['"']='\\"',['\n']='\\n',['\t']='\\t'}
function encodeString(s)
  return tostring(s):gsub('["\\\n\t]',qrep)
end

-- Determines whether the given Lua type is an array or a table / dictionary.
-- We consider any table an array if it has indexes 1..n for its n items, and no
-- other data in the table.
-- I think this method is currently a little 'flaky', but can't think of a good way around it yet...
-- @param t The table to evaluate as an array
-- @return boolean, number True if the table can be represented as an array, false otherwise. If true,
-- the second returned value is the maximum
-- number of indexed elements in the array. 
function isArray(t)
  -- Next we count all the elements, ensuring that any non-indexed elements are not-encodable 
  -- (with the possible exception of 'n')
  local maxIndex = 0
  for k,v in base.pairs(t) do
    if (base.type(k)=='number' and math.floor(k)==k and 1<=k) then	-- k,v is an indexed pair
      if (not isEncodable(v)) then return false end	-- All array elements must be encodable
      maxIndex = math.max(maxIndex,k)
    else
      if (k=='n') then
        if v ~= table.getn(t) then return false end  -- False if n does not hold the number of elements
      else -- Else of (k=='n')
        if isEncodable(v) then return false end
      end  -- End of (k~='n')
    end -- End of k,v not an indexed pair
  end  -- End of loop across all pairs
  return true, maxIndex
end

--- Determines whether the given Lua object / table / variable can be JSON encoded. The only
-- types that are JSON encodable are: string, boolean, number, nil, table and json.null.
-- In this implementation, all other types are ignored.
-- @param o The object to examine.
-- @return boolean True if the object should be JSON encoded, false if it should be ignored.
function isEncodable(o)
  local t = base.type(o)
  return (t=='string' or t=='boolean' or t=='number' or t=='nil' or t=='table') or (t=='function' and o==null) 
end

-- Radical performance improvement for decode from Eike Decker!
do
	local type = base.type
	local error = base.error
	local assert = base.assert
	local print = base.print
	local tonumber = base.tonumber
	-- initialize some values to be used in decoding function
	
	-- initializes a table to contain a byte=>table mapping
	-- the table contains tokens (byte values) as keys and maps them on other
	-- token tables (mostly, the boolean value 'true' is used to indicate termination
	-- of a token sequence)
	-- the token table's purpose is, that it allows scanning a sequence of bytes
	-- until something interesting has been found (e.g. a token that is not expected)
	-- name is a descriptor for the table to be printed in error messages
	local function init_token_table (tt)
		local struct = {}
		local value
		function struct:link(other_tt)
			value = other_tt
			return struct
		end
		function struct:to(chars)
			for i=1,#chars do 
				tt[chars:byte(i)] = value
			end
			return struct
		end
		return function (name)
			tt.name = name
			return struct
		end
	end
	
	-- keep "named" byte values at hands
	local 
		c_esc,
		c_e,
		c_l,
		c_r,
		c_u,
		c_f,
		c_a,
		c_s,
		c_slash = ("\\elrufas/"):byte(1,9)
	
	-- token tables - tt_doublequote_string = strDoubleQuot, tt_singlequote_string = strSingleQuot
	local 
		tt_object_key,
		tt_object_colon,
		tt_object_value,
		tt_doublequote_string,
		tt_singlequote_string,
		tt_array_value,
		tt_array_seperator,
		tt_numeric,
		tt_boolean,
		tt_null,
		tt_comment_start,
		tt_comment_middle,
		tt_ignore --< tt_ignore is special - marked tokens will be tt_ignored
			= {},{},{},{},{},{},{},{},{},{},{},{},{}
	
	-- strings to be used in certain token tables
	local strchars = "" -- all valid string characters (all except newlines)
	local allchars = "" -- all characters that are valid in comments
	--local escapechar = {}
	for i=0,0xff do 
		local c = string.char(i)
		if c~="\n" and c~="\r" then strchars = strchars .. c end
		allchars = allchars .. c
		--escapechar[i] = "\\" .. string.char(i)
	end
	
--[[	
	charstounescape = "\"\'\\bfnrt/";
	unescapechars = "\"'\\\b\f\n\r\t\/";
	for i=1,#charstounescape do
		escapechar[ charstounescape:byte(i) ] = unescapechars:sub(i,i)
	end
]]--

	-- obj key reader, expects the end of the object or a quoted string as key
	init_token_table (tt_object_key) "object (' or \" or } or , expected)" 
		:link(tt_singlequote_string) :to "'"
		:link(tt_doublequote_string) :to '"'
		:link(true)                  :to "}"
		:link(tt_object_key)         :to ","
		:link(tt_comment_start)      :to "/"
		:link(tt_ignore)             :to " \t\r\n"
	
	
	-- after the key, a colon is expected (or comment)
	init_token_table (tt_object_colon) "object (: expected)" 
		:link(tt_object_value)       :to ":"  
		:link(tt_comment_start)      :to "/" 
		:link(tt_ignore)             :to" \t\r\n"
		
	-- as values, anything is possible, numbers, arrays, objects, boolean, null, strings
	init_token_table (tt_object_value) "object ({ or [ or ' or \" or number or boolean or null expected)"
		:link(tt_object_key)         :to "{" 
		:link(tt_array_seperator)    :to "[" 
		:link(tt_singlequote_string) :to "'" 
		:link(tt_doublequote_string) :to '"' 
		:link(tt_numeric)            :to "0123456789.-" 
		:link(tt_boolean)            :to "tf" 
		:link(tt_null)               :to "n" 
		:link(tt_comment_start)      :to "/" 
		:link(tt_ignore)             :to " \t\r\n"
		
	-- token tables for reading strings
	init_token_table (tt_doublequote_string) "double quoted string"
		:link(tt_ignore)             :to (strchars)
		:link(c_esc)                 :to "\\"
		:link(true)                  :to '"'
		
	init_token_table (tt_singlequote_string) "single quoted string"
		:link(tt_ignore)             :to (strchars)
		:link(c_esc)                 :to "\\" 
		:link(true)                  :to "'"
		
	-- array reader that expects termination of the array or a comma that indicates the next value
	init_token_table (tt_array_value) "array (, or ] expected)"
		:link(tt_array_seperator)    :to "," 
		:link(true)                  :to "]"
		:link(tt_comment_start)      :to "/" 
		:link(tt_ignore)             :to " \t\r\n"
	
	-- a value, pretty similar to tt_object_value
	init_token_table (tt_array_seperator) "array ({ or [ or ' or \" or number or boolean or null expected)"
		:link(tt_object_key)         :to "{" 
		:link(tt_array_seperator)    :to "[" 
		:link(tt_singlequote_string) :to "'" 
		:link(tt_doublequote_string) :to '"'  
		:link(tt_comment_start)      :to "/" 
		:link(tt_numeric)            :to "0123456789.-" 
		:link(tt_boolean)            :to "tf" 
		:link(tt_null)               :to "n" 
		:link(tt_ignore)             :to " \t\r\n"
	
	-- valid number tokens
	init_token_table (tt_numeric) "number"
		:link(tt_ignore)             :to "0123456789.-Ee"
		
	-- once a comment has been started with /, a * is expected
	init_token_table (tt_comment_start) "comment start (* expected)"
		:link(tt_comment_middle)     :to "*"
		
	-- now everything is allowed, watch out for * though. The next char is then checked manually
	init_token_table (tt_comment_middle) "comment end"
		:link(tt_ignore)             :to (allchars)
		:link(true)                  :to "*"
		
	function decode (js_string)
		local pos = 1 -- position in the string
		
		-- read the next byte value
		local function next_byte () pos = pos + 1 return js_string:byte(pos-1) end
		
		-- in case of error, report the location using line numbers
		local function location () 
			local n = ("\n"):byte()
			local line,lpos = 1,0
			for i=1,pos do 
				if js_string:byte(i) == n then
					line,lpos = line + 1,1
				else
					lpos = lpos + 1
				end
			end
			return "Line "..line.." character "..lpos
		end
		
		-- debug func
		--local function status (str)
		--	print(str.." ("..s:sub(math.max(1,p-10),p+10)..")")
		--end
		
		-- read the next token, according to the passed token table
		local function next_token (tok)
			while pos <= #js_string do
				local b = js_string:byte(pos) 
				local t = tok[b]
				if not t then 
					error("Unexpected character at "..location()..": "..
						string.char(b).." ("..b..") when reading "..tok.name.."\nContext: \n"..
						js_string:sub(math.max(1,pos-30),pos+30).."\n"..(" "):rep(pos+math.min(-1,30-pos)).."^")
				end
				pos = pos + 1
				if t~=tt_ignore then return t end
			end
			error("unexpected termination of JSON while looking for "..tok.name)
		end
		
		-- read a string, double and single quoted ones
		local function read_string (tok)
			local start = pos
			--local returnString = {}
			repeat
				local t = next_token(tok)
				if t == c_esc then 
					--table.insert(returnString, js_string:sub(start, pos-2))
					--table.insert(returnString, escapechar[ js_string:byte(pos) ])
					pos = pos + 1
					--start = pos
				end -- jump over escaped chars, no matter what
			until t == true
			return (base.loadstring("return " .. js_string:sub(start-1, pos-1) ) ())

			-- We consider the situation where no escaped chars were encountered separately,
			-- and use the fastest possible return in this case.
			
			--if 0 == #returnString then
			--	return js_string:sub(start,pos-2)
			--else
			--	table.insert(returnString, js_string:sub(start,pos-2))
			--	return table.concat(returnString,"");
			--end
			--return js_string:sub(start,pos-2)
		end
		
		local function read_num ()
			local start = pos
			while pos <= #js_string do
				local b = js_string:byte(pos)
				if not tt_numeric[b] then break end
				pos = pos + 1
			end
			return tonumber(js_string:sub(start-1,pos-1))
		end
		
		-- read_bool and read_null are both making an assumption that I have not tested:
		-- I would expect that the string extraction is more expensive than actually 
		-- making manual comparision of the byte values
		local function read_bool () 
			pos = pos + 3
			local a,b,c,d = js_string:byte(pos-3,pos)
			if a == c_r and b == c_u and c == c_e then return true end
			pos = pos + 1
			if a ~= c_a or b ~= c_l or c ~= c_s or d ~= c_e then 
				error("Invalid boolean: "..js_string:sub(math.max(1,pos-5),pos+5)) 
			end
			return false
		end
		
		-- same as read_bool: only last 
		local function read_null ()
			pos = pos + 3
			local u,l1,l2 = js_string:byte(pos-3,pos-1)
			if u == c_u and l1 == c_l and l2 == c_l then return nil end
			error("Invalid value (expected null):"..js_string:sub(pos-4,pos-1)..
				" ("..js_string:byte(pos-1).."="..js_string:sub(pos-1,pos-1).." / "..c_l..")")
		end
		
		local read_object_value,read_object_key,read_array,read_value,read_comment
	
		-- read a value depending on what token was returned, might require info what was used (in case of comments)
		function read_value (t,fromt)
			if t == tt_object_key         then return read_object_key({}) end
			if t == tt_array_seperator    then return read_array({}) end
			if t == tt_singlequote_string or 
			   t == tt_doublequote_string then return read_string(t) end
			if t == tt_numeric            then return read_num() end
			if t == tt_boolean            then return read_bool() end	
			if t == tt_null               then return read_null() end
			if t == tt_comment_start      then return read_value(read_comment(fromt)) end
			error("unexpected termination - "..js_string:sub(math.max(1,pos-10),pos+10))
		end
		
		-- read comments until something noncomment like surfaces, using the token reader which was 
		-- used when stumbling over this comment
		function read_comment (fromt)
			while true do
				next_token(tt_comment_start)
				while true do
					local t = next_token(tt_comment_middle)
					if next_byte() == c_slash then
						local t = next_token(fromt)
						if t~= tt_comment_start then return t end
						break
					end
				end
			end
		end
		
		-- read arrays, empty array expected as o arg
		function read_array (o,i)
			--if not i then status "arr open" end
			i = i or 1
			-- loop until ...
			while true do
				o[i] = read_value(next_token(tt_array_seperator),tt_array_seperator)
				local t = next_token(tt_array_value)
				if t == tt_comment_start then
					t = read_comment(tt_array_value)
				end
				if t == true then  -- ... we found a terminator token
					--status "arr close"
					return o 
				end
				i = i + 1			
			end
		end
		
		-- object value reading
		function read_object_value (o)
			local t = next_token(tt_object_value)
			return read_value(t,tt_object_value)
		end
		
		-- object key reading, might also terminate the object
		function read_object_key (o)
			while true do
				local t = next_token(tt_object_key)
				if t == tt_comment_start then
					t = read_comment(tt_object_key)
				end
				if t == true then return o end
				if t == tt_object_key then return read_object_key(o) end
				local k = read_string(t)
				
				if next_token(tt_object_colon) == tt_comment_start then
					t = read_comment(tt_object_colon)
				end
				
				local v = read_object_value(o)
				o[k] = v
			end
		end
		
		-- now let's read data from our string and pretend it's an object value
		local r = read_object_value()
		if pos<=#js_string then
			-- not sure about what to do with dangling characters
			--error("Dangling characters in JSON code ("..location()..")")
		end
		
		return r
	end
end
