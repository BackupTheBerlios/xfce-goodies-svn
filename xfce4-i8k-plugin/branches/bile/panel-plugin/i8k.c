/*
 * Copyright (c) 2004 Antonio SJ Musumeci <bile@landofbile.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.1
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_clock.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#define PROC_I8K "/proc/i8k"

const static gchar* fan_image[] = { "gtk-close",
				    "gtk-redo",
				    "gtk-refresh" };
const static int image_size[] = {GTK_ICON_SIZE_MENU, 
				 GTK_ICON_SIZE_MENU,
				 GTK_ICON_SIZE_BUTTON,
				 GTK_ICON_SIZE_DND};
struct i8k_t
{
  int automatic;
  int set_exact_speeds;
  int fan1_speed;
  int fan2_speed;
  int cpu_temp;
  int max_off;
  int max_low;
  int icon_size;
  int timeout_id;
  char fan_controller[256];
  GtkWidget* plugin;
  GtkWidget* temp_label;
  GtkWidget* fan_box;
  GtkWidget* fan1_button;
  GtkWidget* fan1_image;
  GtkWidget* fan2_button;
  GtkWidget* fan2_image;
};

static void get_i8k_info(struct i8k_t* i8k)
{
  int rv;
  gchar* t;
  gchar i8k_data[128];
  FILE* file_ptr;

  file_ptr = fopen(PROC_I8K, "r");
  if(file_ptr == NULL)
    {
      i8k->fan1_speed = 2;
      i8k->fan2_speed = 2;
      i8k->cpu_temp = 50;
    }

  rv = fread(i8k_data, sizeof(gchar), 128, file_ptr);
  if(rv == 0)
    {
      i8k->fan1_speed = 2;
      i8k->fan2_speed = 2;
      i8k->cpu_temp = 0;
    }
  fclose(file_ptr);

  /* version */
  t = strtok(i8k_data, " ");
  /* bios */
  t = strtok(NULL, " ");
  /* serial number */
  t = strtok(NULL, " ");
  /* cpu temp */
  t = strtok(NULL, " ");
  i8k->cpu_temp = atoi(t);
  /* left fan status */
  t = strtok(NULL, " ");
  i8k->fan1_speed = atoi(t);
  /* right fan status */
  t = strtok(NULL, " ");
  i8k->fan2_speed = atoi(t);
  /* we dont care about the rest */
}

static void set_fan1(struct i8k_t* i8k)
{
  gchar command_buffer[256];

  snprintf(command_buffer, 256, "%s %d - > /dev/null", i8k->fan_controller, i8k->fan1_speed);
  system(command_buffer);
}

static void set_fan2(struct i8k_t* i8k)
{
  gchar command_buffer[256];
  
  snprintf(command_buffer, 256, "%s - %d > /dev/null", i8k->fan_controller, i8k->fan2_speed);
  system(command_buffer);
}

static void fan1_button_clicked_callback(GtkWidget *button, gpointer data)
{
  struct i8k_t* i8k;
 
  i8k = (struct i8k_t*)data;

  i8k->fan1_speed++;
  if(i8k->fan1_speed > 2)
    i8k->fan1_speed = 0;
  set_fan1(i8k);
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan1_image), fan_image[i8k->fan1_speed], i8k->icon_size);
}

static void fan2_button_clicked_callback(GtkWidget *button, gpointer data)
{
  struct i8k_t* i8k;
 
  i8k = (struct i8k_t*)data;

  i8k->fan2_speed++;
  if(i8k->fan2_speed > 2)
    i8k->fan2_speed = 0;
  set_fan2(i8k);
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan2_image), fan_image[i8k->fan2_speed], i8k->icon_size);
}

static gboolean update_i8k(struct i8k_t* i8k)
{
  static gchar temp_string[16];
  static int old_temp;
  static int fan1_speed;
  static int fan2_speed;

  old_temp = i8k->cpu_temp;
  fan1_speed = i8k->fan1_speed;
  fan2_speed = i8k->fan2_speed;

  get_i8k_info(i8k);

  if(old_temp != i8k->cpu_temp)
    {
      snprintf(temp_string, 16, "%d C", i8k->cpu_temp);
      gtk_label_set_text(GTK_LABEL(i8k->temp_label), temp_string);
    }

  if(i8k->automatic)
    {
      if(i8k->cpu_temp >= i8k->max_low)
	{
	  i8k->fan1_speed = 2;
	  i8k->fan2_speed = 2;
	  set_fan1(i8k);
	  set_fan2(i8k);
	}
      else if(i8k->cpu_temp >= i8k->max_off)
	{
	  if(i8k->set_exact_speeds ||
	     i8k->fan1_speed == 0)
	    {
	      i8k->fan1_speed = 1;
	      set_fan1(i8k);
	    }
	  
	  if(i8k->set_exact_speeds ||
	     i8k->fan2_speed == 0)
	    {
	      i8k->fan2_speed = 1;
	      set_fan2(i8k);
	    }
	}
      else if(i8k->set_exact_speeds)
	{
	  i8k->fan1_speed = 0;
	  i8k->fan2_speed = 0;
	  set_fan1(i8k);
	  set_fan2(i8k);
	}
    } 
 
  if(fan1_speed != i8k->fan1_speed)
    gtk_image_set_from_stock(GTK_IMAGE(i8k->fan1_image), fan_image[i8k->fan1_speed], i8k->icon_size);
  if(fan2_speed != i8k->fan2_speed)
    gtk_image_set_from_stock(GTK_IMAGE(i8k->fan2_image), fan_image[i8k->fan2_speed], i8k->icon_size);

  return TRUE;
}

static struct i8k_t* i8k_new(void)
{
  struct i8k_t* i8k;
  gchar temp_string[16];
  
  i8k = g_new(struct i8k_t, 1);

  get_i8k_info(i8k);

  i8k->plugin = gtk_vbox_new(FALSE, 1);
  {
    snprintf(temp_string, 16, "%d C", i8k->cpu_temp);
    i8k->temp_label = gtk_label_new(temp_string);
    gtk_container_add(GTK_CONTAINER(i8k->plugin), i8k->temp_label);
    
    i8k->fan_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(i8k->plugin), i8k->fan_box);
    {
      i8k->fan1_button = gtk_button_new();

      gtk_button_set_relief(GTK_BUTTON(i8k->fan1_button), GTK_RELIEF_NONE);
      gtk_container_add(GTK_CONTAINER(i8k->fan_box), i8k->fan1_button);
      {
	i8k->fan1_image = gtk_image_new_from_stock(fan_image[i8k->fan1_speed], i8k->icon_size);
	gtk_container_add(GTK_CONTAINER(i8k->fan1_button), i8k->fan1_image);
      }

      i8k->fan2_button = gtk_button_new();
      gtk_container_add(GTK_CONTAINER(i8k->fan_box), i8k->fan2_button);
      gtk_button_set_relief(GTK_BUTTON(i8k->fan2_button), GTK_RELIEF_NONE);
      {
	i8k->fan2_image = gtk_image_new_from_stock(fan_image[i8k->fan2_speed], i8k->icon_size);
	gtk_container_add(GTK_CONTAINER(i8k->fan2_button), i8k->fan2_image);
      }
    }
  }
  
  gtk_widget_show_all(i8k->plugin);

  g_signal_connect(i8k->fan1_button, "clicked",
		   G_CALLBACK(fan1_button_clicked_callback), i8k);
  g_signal_connect(i8k->fan2_button, "clicked",
		   G_CALLBACK(fan2_button_clicked_callback), i8k);
  return i8k;
}

static gboolean i8k_control_new(Control* control)
{
  struct i8k_t* i8k;

  i8k = i8k_new();

  strcpy(i8k->fan_controller, "/usr/bin/i8kfan");

  i8k->timeout_id = g_timeout_add(2500, (GSourceFunc)update_i8k, i8k);

  gtk_container_add(GTK_CONTAINER(control->base), i8k->plugin);
  
  control->data = (gpointer)i8k;
  control->with_popup = FALSE;

  gtk_widget_set_size_request(control->base, -1, -1);

  return TRUE;
}

static void i8k_free(Control* control)
{
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;
  if(i8k->timeout_id > 0)
    g_source_remove(i8k->timeout_id);
  g_free(i8k);
}

static void i8k_read_config(Control* control, xmlNodePtr node)
{
  xmlChar* value;
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  if(!node || !node->children)
    return;

  node = node->children;
  if(!xmlStrEqual(node->name, "I8K"))
    return;

  value = xmlGetProp(node, (const xmlChar*) "Max_Off");
  if(value)
    {
      i8k->max_off = atoi(value);
      g_free(value);
    }

  value = xmlGetProp(node, (const xmlChar*) "Max_Low");
  if(value)
    {
      i8k->max_low = atoi(value);
      g_free(value);
    }

  value = xmlGetProp(node, (const xmlChar*) "Automatic");
  if(value)
    {
      i8k->automatic = atoi(value);
      g_free(value);
    }

  value = xmlGetProp(node, (const xmlChar*) "Set_Exact_Speeds");
  if(value)
    {
      i8k->set_exact_speeds = atoi(value);
      g_free(value);
    }
}

static void i8k_write_config(Control *control, xmlNodePtr parent)
{
  xmlNodePtr root;
  gchar value[MAXSTRLEN + 1];
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  root = xmlNewTextChild(parent, NULL, "I8K", NULL);
  g_snprintf(value, 3, "%d", i8k->max_off);
  xmlSetProp(root, "Max_Off", value);
  g_snprintf(value, 3, "%d", i8k->max_low);
  xmlSetProp(root, "Max_Low", value);
  g_snprintf(value, 3, "%d", i8k->automatic);
  xmlSetProp(root, "Automatic", value);
  g_snprintf(value, 3, "%d", i8k->set_exact_speeds);
  xmlSetProp(root, "Set_Exact_Speeds", value);
}

static void i8k_attach_callback(Control *control, const gchar* signal, GCallback callback, gpointer data)
{
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  g_signal_connect(i8k->plugin, signal, callback, data);
}

static void i8k_set_size(Control* control, int size)
{
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  if(size < 0)
    size = 0;
  else if(size > 4)
    size = 4;

  i8k->icon_size = image_size[size];
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan1_image), fan_image[i8k->fan1_speed], i8k->icon_size);
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan2_image), fan_image[i8k->fan2_speed], i8k->icon_size);
}

static void update_max_off(GtkSpinButton* spin_button, struct i8k_t* i8k)
{
  i8k->max_off = gtk_spin_button_get_value_as_int(spin_button);
}

static void update_max_low(GtkSpinButton* spin_button, struct i8k_t* i8k)
{
  i8k->max_low = gtk_spin_button_get_value_as_int(spin_button);
}

static void update_auto_fan(GtkToggleButton* check_button, struct i8k_t* i8k)
{
  i8k->automatic = gtk_toggle_button_get_active(check_button);
}

static void update_set_exact_speeds(GtkToggleButton* check_button, struct i8k_t* i8k)
{
  i8k->set_exact_speeds = gtk_toggle_button_get_active(check_button);
}

static void i8k_create_options (Control* control, GtkContainer* container, GtkWidget* done)
{
  struct i8k_t* i8k;
  GtkWidget* main_box;
  GtkWidget* hbox;
  GtkWidget* label;
  GtkWidget* button;

  i8k = (struct i8k_t*)control->data;
  
  main_box = gtk_vbox_new(FALSE, 2);
  {
    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(main_box), hbox);
    {
      label = gtk_label_new("Set exact speeds: ");
      gtk_container_add(GTK_CONTAINER(hbox), label);

      button = gtk_check_button_new();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), i8k->set_exact_speeds);
      g_signal_connect(button, "toggled", G_CALLBACK(update_set_exact_speeds), i8k);
      gtk_container_add(GTK_CONTAINER(hbox), button);
    }

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(main_box), hbox);
    {
      label = gtk_label_new("Automatic fan control: ");
      gtk_container_add(GTK_CONTAINER(hbox), label);
      
      button = gtk_check_button_new();
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), i8k->automatic);
      g_signal_connect(button, "toggled", G_CALLBACK(update_auto_fan), i8k);
      gtk_container_add(GTK_CONTAINER(hbox), button);
    }

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(main_box), hbox);
    {
      label = gtk_label_new(" Temp fans turn on low: ");
      gtk_container_add(GTK_CONTAINER(hbox), label);

      button = gtk_spin_button_new_with_range(0.0, 100.0, 1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), (double)i8k->max_off);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(button), 0);
      g_signal_connect(button, "value_changed", G_CALLBACK(update_max_off), i8k);
      gtk_container_add(GTK_CONTAINER(hbox), button);
    }

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(main_box), hbox);
    {
      label = gtk_label_new("Temp fans turn on high: ");
      gtk_container_add(GTK_CONTAINER(hbox), label);

      button = gtk_spin_button_new_with_range(0.0, 100.0, 1.0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), (double)i8k->max_low);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(button), 0);
      g_signal_connect(button, "value_changed", G_CALLBACK(update_max_low), i8k);
      gtk_container_add(GTK_CONTAINER(hbox), button);      
    }
  }
  
  gtk_widget_show_all(main_box);
  gtk_container_add(container, main_box);
}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc)
{
  cc->name		= "i8k";
  cc->caption		= _("i8k plugin");
  cc->create_control	= (CreateControlFunc)i8k_control_new;
  cc->free		= i8k_free;
  cc->attach_callback	= i8k_attach_callback;
  cc->read_config	= i8k_read_config;
  cc->write_config	= i8k_write_config;
  cc->create_options	= i8k_create_options;
  cc->set_size		= i8k_set_size;
}

XFCE_PLUGIN_CHECK_INIT
