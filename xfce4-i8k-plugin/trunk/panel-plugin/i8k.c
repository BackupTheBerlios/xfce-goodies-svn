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

#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_clock.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#include "i8k.h"

const static gchar* fan_image[] = 
  { 
    "gtk-close",
    "gtk-redo",
    "gtk-refresh"
  };
const static int image_size[] = {GTK_ICON_SIZE_MENU, 
				 GTK_ICON_SIZE_MENU,
				 GTK_ICON_SIZE_BUTTON,
				 GTK_ICON_SIZE_DND};
typedef struct i8k_t i8k_t;
struct i8k_t
{
  int fd;
  int automatic;
  int set_exact_speeds;
  int max_off;
  int max_low;
  int icon_size;
  int timeout_id;
  float update_rate;
  GtkWidget* plugin;
  GtkWidget* temp_label;
  GtkWidget* fan_box;
  GtkWidget* fan_button[2];
  GtkWidget* fan_image[2];
};

static int
i8k_set_fan(i8k_t* i8k, int fan, int speed)
{
  int rv;
  int args[2];

  if(i8k->fd >= 0)
    {
      args[0] = fan;
      args[1] = speed;
      rv = ioctl(i8k->fd, I8K_SET_FAN, &args);
      if(rv < 0)
	return rv;
      
      return args[0];
    }
  
  return -1;
}

static int
i8k_get_fan(i8k_t* i8k, int fan)
{
  int rv;
  int args[1];

  if(i8k->fd >= 0)
    {
      args[0] = fan;
      rv = ioctl(i8k->fd, I8K_GET_FAN, &args);
      if(rv < 0)
	return rv;
  
      return args[0];
    }

  return -1;
}

static int
i8k_get_cpu_temp(i8k_t* i8k)
{
  int rv;
  int args[1];
  
  if(i8k->fd >= 0)
    {
      rv = ioctl(i8k->fd, I8K_GET_TEMP, &args);
      if(rv < 0)
	return rv;
      
      return args[0];
    }
  
  return -1;
}

static void
fan0_button_clicked_callback(GtkWidget *button, gpointer data)
{
  int rv;
  int speed;
  i8k_t* i8k = data;

  speed = (i8k_get_fan(i8k, I8K_FAN_LEFT) + 1);
  if(speed > I8K_FAN_HIGH)
    speed = I8K_FAN_OFF;
  
  rv = i8k_set_fan(i8k, I8K_FAN_LEFT, speed);
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan_image[I8K_FAN_LEFT]), 
			   fan_image[(rv < 0) ? 0 : speed],
			   i8k->icon_size);
}

static void
fan1_button_clicked_callback(GtkWidget *button, gpointer data)
{
  int rv;
  int speed;
  i8k_t* i8k = data;

  speed = (i8k_get_fan(i8k, I8K_FAN_RIGHT) + 1);
  if(speed > I8K_FAN_HIGH)
    speed = I8K_FAN_OFF;
  
  rv = i8k_set_fan(i8k, I8K_FAN_RIGHT, speed);
  gtk_image_set_from_stock(GTK_IMAGE(i8k->fan_image[I8K_FAN_RIGHT]), 
			   fan_image[(rv < 0) ? 0 : speed],
			   i8k->icon_size);
}

static gboolean
update_i8k(i8k_t* i8k)
{
  int i;
  int cpu_temp;
  int ofs[2];
  gchar temp_string[16];

  ofs[I8K_FAN_LEFT]  = i8k_get_fan(i8k, I8K_FAN_LEFT);
  ofs[I8K_FAN_RIGHT] = i8k_get_fan(i8k, I8K_FAN_RIGHT);

  cpu_temp = i8k_get_cpu_temp(i8k);
  snprintf(temp_string, 16, "%d C", cpu_temp);
  gtk_label_set_text(GTK_LABEL(i8k->temp_label), temp_string);
  if(i8k->automatic)
    {
      if(cpu_temp >= i8k->max_low)
	{
	  i8k_set_fan(i8k, I8K_FAN_LEFT, I8K_FAN_HIGH);
	  i8k_set_fan(i8k, I8K_FAN_RIGHT, I8K_FAN_HIGH);
	}
      else if(cpu_temp >= i8k->max_off)
	{
	  for(i = 0; i < 2; i++)
	    {
	      if(i8k->set_exact_speeds || ofs[i] ==  I8K_FAN_OFF)
		i8k_set_fan(i8k, i, I8K_FAN_LOW);
	    }
	}
      else if(i8k->set_exact_speeds)
	{
	  i8k_set_fan(i8k, I8K_FAN_LEFT, I8K_FAN_OFF);
	  i8k_set_fan(i8k, I8K_FAN_RIGHT, I8K_FAN_OFF);
	}
    } 
 
  for(i = 0; i < 2; i++)
    {
      int speed = i8k_get_fan(i8k, i);

      if(speed < 0)
	speed = 0;
      gtk_image_set_from_stock(GTK_IMAGE(i8k->fan_image[i]), fan_image[speed], i8k->icon_size);
    }

  return TRUE;
}

static i8k_t*
i8k_new(void)
{
  int i;
  i8k_t* i8k;
  gchar temp_string[16];
  
  i8k = g_new(struct i8k_t, 1);

  i8k->fd = open(I8K_PROC, O_RDONLY);

  i8k->plugin = gtk_vbox_new(FALSE, 1);
  {
    snprintf(temp_string, 16, "%d C", i8k_get_cpu_temp(i8k));
    i8k->temp_label = gtk_label_new(temp_string);
    gtk_container_add(GTK_CONTAINER(i8k->plugin), i8k->temp_label);
    
    i8k->fan_box = gtk_hbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(i8k->plugin), i8k->fan_box);
    {
      for(i = 0; i < 2; i++)
	{
	  int speed = i8k_get_fan(i8k, i);
	  i8k->fan_button[i] = gtk_button_new();

	  gtk_button_set_relief(GTK_BUTTON(i8k->fan_button[i]), GTK_RELIEF_NONE);
	  gtk_container_add(GTK_CONTAINER(i8k->fan_box), i8k->fan_button[i]);
	  {
	    i8k->fan_image[i] = gtk_image_new_from_stock(fan_image[speed], i8k->icon_size);
	    gtk_container_add(GTK_CONTAINER(i8k->fan_button[i]), i8k->fan_image[i]);
	  }
	}
    }
  }
  
  gtk_widget_show_all(i8k->plugin);

  g_signal_connect(i8k->fan_button[0], "clicked",
		   G_CALLBACK(fan0_button_clicked_callback), i8k);
  g_signal_connect(i8k->fan_button[1], "clicked",
		   G_CALLBACK(fan1_button_clicked_callback), i8k);

  return i8k;
}

static gboolean
i8k_control_new(Control* control)
{
  i8k_t* i8k;

  i8k = i8k_new();

  gtk_container_add(GTK_CONTAINER(control->base), i8k->plugin);
  
  control->data = (gpointer)i8k;
  control->with_popup = FALSE;

  gtk_widget_set_size_request(control->base, -1, -1);

  return TRUE;
}

static void i8k_free(Control* control)
{
  i8k_t* i8k = (i8k_t*)control->data;

  if(i8k->timeout_id > 0)
    g_source_remove(i8k->timeout_id);

  g_free(i8k);
}

static void
i8k_read_config(Control* control, xmlNodePtr node)
{
  xmlChar* value;
  i8k_t* i8k = (i8k_t*)control->data;

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

  value = xmlGetProp(node, (const xmlChar*) "Update_Rate");
  if(value)
    {
      i8k->update_rate = atof(value);
      g_free(value);
    }
  else
    i8k->update_rate = 2.5;

  i8k->timeout_id = g_timeout_add((int)(i8k->update_rate * 1000.0), (GSourceFunc)update_i8k, i8k);
}

static void
i8k_write_config(Control *control, xmlNodePtr parent)
{
  xmlNodePtr root;
  gchar value[MAXSTRLEN + 1];
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  root = xmlNewTextChild(parent, NULL, "I8K", NULL);
  g_snprintf(value, 6, "%d", i8k->max_off);
  xmlSetProp(root, "Max_Off", value);
  g_snprintf(value, 6, "%d", i8k->max_low);
  xmlSetProp(root, "Max_Low", value);
  g_snprintf(value, 3, "%d", i8k->automatic);
  xmlSetProp(root, "Automatic", value);
  g_snprintf(value, 3, "%d", i8k->set_exact_speeds);
  xmlSetProp(root, "Set_Exact_Speeds", value);
  g_snprintf(value, MAXSTRLEN, "%f", i8k->update_rate);
  xmlSetProp(root, "Update_Rate", value);
}

static void
i8k_attach_callback(Control *control, const gchar* signal, GCallback callback, gpointer data)
{
  struct i8k_t* i8k;

  i8k = (struct i8k_t*)control->data;

  g_signal_connect(i8k->plugin, signal, callback, data);
}

static void
i8k_set_size(Control* control, int size)
{
  struct i8k_t* i8k;
  int speed, i;

  i8k = (struct i8k_t*)control->data;

  if(size < 0)
    size = 0;
  else if(size > 4)
    size = 4;

  i8k->icon_size = image_size[size];
  for(i = 0; i < 2; i++)
    {
      speed = i8k_get_fan(i8k, i);
      if(speed < 0)
	speed = 0;
      gtk_image_set_from_stock(GTK_IMAGE(i8k->fan_image[i]), fan_image[speed], i8k->icon_size);
    }
}

static void
update_max_off(GtkSpinButton* spin_button, struct i8k_t* i8k)
{
  i8k->max_off = gtk_spin_button_get_value_as_int(spin_button);
}

static void
update_max_low(GtkSpinButton* spin_button, struct i8k_t* i8k)
{
  i8k->max_low = gtk_spin_button_get_value_as_int(spin_button);
}

static void
update_auto_fan(GtkToggleButton* check_button, struct i8k_t* i8k)
{
  i8k->automatic = gtk_toggle_button_get_active(check_button);
}

static void
update_set_exact_speeds(GtkToggleButton* check_button, struct i8k_t* i8k)
{
  i8k->set_exact_speeds = gtk_toggle_button_get_active(check_button);
}

static void
update_update_rate(GtkSpinButton* spin_button, i8k_t* i8k)
{
  i8k->update_rate = gtk_spin_button_get_value_as_float(spin_button);
  gtk_timeout_remove(i8k->timeout_id);
  i8k->timeout_id = gtk_timeout_add((int)(i8k->update_rate * 1000.0), (GSourceFunc)update_i8k, i8k);
}

static void
i8k_create_options (Control* control, GtkContainer* container, GtkWidget* done)
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
      label = gtk_label_new("Update rate:");
      gtk_container_add(GTK_CONTAINER(hbox), label);
      
      button = gtk_spin_button_new_with_range(0.0, 100.0, 0.1);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), (float)i8k->update_rate);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(button), 1);
      g_signal_connect(button, "value_changed", G_CALLBACK(update_update_rate), i8k);
      gtk_container_add(GTK_CONTAINER(hbox), button);
    }

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
