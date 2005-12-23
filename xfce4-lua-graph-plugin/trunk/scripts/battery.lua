--
--battert with AC indicator
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
	add_progressbar("prg",0,0,0);
	
	max_bat_charge = read_numeric_value_from_file(
		"/proc/acpi/battery/BAT0/info","last full capacity");
end

function update()
	local current_charge = read_numeric_value_from_file(
		"/proc/acpi/battery/BAT0/state","remaining capacity");
		
	local ac = read_line_from_file("/proc/acpi/ac_adapter/AC/state");
	local i = string.find(ac,"on");
	
	local state = "<span background=\"#FF009D\">BAT</span>" ;
	if i ~= nil then
		state = "AC";
	end;
	
	percent = current_charge*100/max_bat_charge;
	set_label_text( "lbl", small_text(state .. "\n")..string.format("%d%%", percent)) ;
	set_progress_bar_value( "prg", percent ) ;
	set_progress_bar_color( "prg", get_status_color(percent) ) ;
end

