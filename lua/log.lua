local log = {

  output_path = "./config/lua-log.log",
  datetime_format= "%Y-%m-%d %H:%M:%S"

}

log.init = function(output_path)

  if (output_path ~= nil) then
    log.output_path = output_path
  end
  return log

end

log.clear = function()

    local output_file = io.open (log.output_path, "w+")
    io.close(output_file)

end

log.message = function(message_text, ...)

  local message_text_formatted = ""

  if (#{...} > 0) then
    message_text_formatted = string.format(message_text, ...)
  else
    message_text_formatted = tostring(message_text)
  end

  local output_text = os.date(log.datetime_format).."|"..message_text_formatted.."\n"
  local output_file = io.open (log.output_path, "a")

  io.output(output_file)
  io.write(output_text)
  io.close(output_file)

end

return log
