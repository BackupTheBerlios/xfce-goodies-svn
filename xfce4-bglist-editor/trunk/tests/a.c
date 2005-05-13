#include <gtk/gtk.h>
#include <string.h>

int main(int argc, char *argv[])
{
	gchar *ex[2] = {
		"xfce-get-background",
		NULL
	};
	gchar *out;
	
	gtk_init (&argc, &argv);
	
	out = NULL;	
	if (g_spawn_sync (
		NULL, ex, NULL, 
		G_SPAWN_SEARCH_PATH,
		NULL, NULL, &out,
		NULL, NULL, NULL))
	{
		if (out && out[0] && g_str_has_suffix (out, "\n")) 
			out[strlen(out) - 1] = 0;
			
		printf("%s\n", out);
	}
	g_free (out);
		
	
	return 0;
}
