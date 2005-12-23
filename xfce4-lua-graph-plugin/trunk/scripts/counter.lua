--
--Simply Counter Demo
--
function init()
	print("Hello from Lua")
	add_label("lbl","<span foreground=\"blue\" size=\"x-large\">Blue text</span> is <i>cool</i>!");
	add_progressbar("prg1",65535,0,65535);
	
	val = 1 ;
end

function update()
	print("Update Called");
	val = val + 1 ;
	
	set_progress_bar_color("prg1",get_status_color(val));
	
	if val>100 then
		val = 100
	end
	set_progress_bar_value("prg1",val);
	
	set_label_text("lbl",large_text("val = \n") .. small_text(string.format("%d", val))) 
end

