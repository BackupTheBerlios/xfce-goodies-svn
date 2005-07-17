/*
 * Copyright (C) 1999 Dave Camp <dave@davec.dhs.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include "eyes.h"

gchar *theme_directories[] = {
        THEMESDIR
};
#define NUM_THEME_DIRECTORIES 1

static void
parse_theme_file (t_eyes *eyes_applet, FILE *theme_file)
{
        gchar line_buf [512]; /* prolly overkill */
        gchar *token;
        fgets (line_buf, 512, theme_file);
        while (!feof (theme_file)) {
                token = strtok (line_buf, "=");
                if (strncmp (token, "wall-thickness", 
                             strlen ("wall-thickness")) == 0) {
                        token += strlen ("wall-thickness");
                        while (!isdigit (*token)) {
                                token++;
                        }
                        sscanf (token, "%d", &eyes_applet->wall_thickness); 
                } else if (strncmp (token, "num-eyes", strlen ("num-eyes")) == 0) {
                        token += strlen ("num-eyes");
                        while (!isdigit (*token)) {
                                token++;
                        }
                        sscanf (token, "%d", &eyes_applet->num_eyes);
                } else if (strncmp (token, "eye-pixmap", strlen ("eye-pixmap")) == 0) {
                        token = strtok (NULL, "\"");
                        token = strtok (NULL, "\"");          
                        if (eyes_applet->eye_filename != NULL) 
                                g_free (eyes_applet->eye_filename);
                        eyes_applet->eye_filename = g_strdup_printf ("%s%s",
                                                                    eyes_applet->theme_dir,
                                                                    token);
                } else if (strncmp (token, "pupil-pixmap", strlen ("pupil-pixmap")) == 0) {
                        token = strtok (NULL, "\"");
                        token = strtok (NULL, "\"");      
            if (eyes_applet->pupil_filename != NULL) 
                    g_free (eyes_applet->pupil_filename);
            eyes_applet->pupil_filename 
                    = g_strdup_printf ("%s%s",
                                       eyes_applet->theme_dir,
                                       token);   
                }
                fgets (line_buf, 512, theme_file);
        }       
}

void
load_theme (t_eyes *eyes_applet, const gchar *theme_dir)
{
	FILE* theme_file;
        gchar *file_name;

        eyes_applet->theme_dir = g_strdup_printf ("%s/", theme_dir);

        file_name = g_strdup_printf("%s%s",theme_dir,"/config");
        theme_file = fopen (file_name, "r");
        if (theme_file == NULL) {
                g_error ("Unable to open theme file.");
        }
        
        parse_theme_file (eyes_applet, theme_file);
        fclose (theme_file);

        eyes_applet->theme_name = g_strdup (theme_dir);
       
        if (eyes_applet->eye_image)
        	g_object_unref (eyes_applet->eye_image);
        eyes_applet->eye_image = gdk_pixbuf_new_from_file (eyes_applet->eye_filename, NULL);
        if (eyes_applet->pupil_image)
        	g_object_unref (eyes_applet->pupil_image);
        eyes_applet->pupil_image = gdk_pixbuf_new_from_file (eyes_applet->pupil_filename, NULL);

	eyes_applet->eye_height = gdk_pixbuf_get_height (eyes_applet->eye_image);
        eyes_applet->eye_width = gdk_pixbuf_get_width (eyes_applet->eye_image);
        eyes_applet->pupil_height = gdk_pixbuf_get_height (eyes_applet->pupil_image);
        eyes_applet->pupil_width = gdk_pixbuf_get_width (eyes_applet->pupil_image);
        
        g_free (file_name);
        
}

#if 0
static void
destroy_theme (t_eyes *eyes_applet)
{
	/* Dunno about this - to unref or not to unref? */
	if (eyes_applet->eye_image != NULL) {
        	g_object_unref(eyes_applet->eye_image); 
        	eyes_applet->eye_image = NULL;
        }
        if (eyes_applet->pupil_image != NULL) {
        	g_object_unref(eyes_applet->pupil_image); 
        	eyes_applet->pupil_image = NULL;
	}
	
        g_free (eyes_applet->theme_dir);
        g_free (eyes_applet->theme_name);
}
#endif
