--
--Temperature ACPI Sensors
--


-- Helper function: read a specific line from a file
function read_numeric_value_from_file(filename,key)
	local info = io.open(filename,"r")
	
	if info == nil then
		return nil
	end
	
	local result ;
	while true do
		local line = info:read()
		if line == nil then break end
		
		local i,j = string.find(line,key) 
		if i ~= nil then
			i,j = string.find(line, "%d+");
			result = string.sub(line,i,j);
		end
	end
	info:close()
	
	return result;
end

-- Helper function: read a the first line of a file
function read_line_from_file(filename)
	local info = io.open(filename,"r")
	
	if info==nil then
		return nil
	end
	
	local line = info:read()
	info:close()
	return line;
end

function init()
	add_label("lbl", "");
	
	convert_to_fahrenheit = nil
	
	units = "c"
	if convert_to_fahrenheit then
		units = "f"
	end
end

function update()
	local value = read_numeric_value_from_file(
		"/proc/acpi/thermal_zone/THM0/temperature","temperature");
	
	if convert_to_fahrenheit then
		value = (value*9/5) + 32 ;
	end
	set_label_text( "lbl", 
		large_text(string.format("%d",value)) .. 
		string.format("<span rise=\"10000\">%s</span>",
		units))
end
