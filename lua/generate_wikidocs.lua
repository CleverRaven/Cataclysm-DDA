dofile "class_definitions.lua"

io.write("=== Global Functions ===\n")

function write_function(name, func)
    io.write("* ")
    io.write("'''"..(func.rval or "void").."'''")
    io.write(" ")
    io.write(name)
    io.write("(")
    for key, arg in ipairs(func.args) do
        io.write(arg)
        if func.argnames then
            io.write(" "..func.argnames[key])
        end
        if next(func.args, key) then
            io.write(", ")
        end
    end
    io.write(")\n")
    
    if func.desc then
        io.write("** "..func.desc.."\n")
    end
end

function write_attribute(name, attribute)
    io.write("* ")
    io.write("'''"..attribute.type.."'''")
    io.write(" ")
    io.write(name)
    if not attribute.writable then
        io.write(" ''read-only''")
    end
    io.write("\n")
    
    if attribute.desc then
        io.write("** "..attribute.desc.."\n")
    end
end

for name, func in pairs(global_functions) do
    write_function(name, func)
end

for classname, class in pairs(classes) do
    io.write("=== "..classname.." ===\n")
    
    io.write("==== Functions ====\n")
    for name, func in pairs(class.functions) do
        write_function(name, func)
    end
    
    io.write("==== Attributes ====\n")
    for name, attr in pairs(class.attributes) do
        write_attribute(name, attr)
    end
end
