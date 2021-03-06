/***************************************************************************
 *            verve.c
 *
 *  $Id$
 *  Copyright  2006  Jannis Pohlmann
 *  info@sten-net.de
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <glib-object.h>
#include <pcre.h>
#include <libxfcegui4/libxfcegui4.h>
#include "verve.h"
#include "verve-env.h"
#include "verve-db.h"
#include "verve-history.h"

/* Internal functions */
gboolean _verve_is_url (const gchar *str);
gboolean _verve_is_email (const gchar *str);

/* URL/eMail matching patterns */
#define USERCHARS       "-A-Za-z0-9"
#define PASSCHARS       "-A-Za-z0-9,?;.:/!%$^*&~\"#'"
#define HOSTCHARS       "-A-Za-z0-9"
#define USER            "[" USERCHARS "]+(:["PASSCHARS "]+)?"
#define MATCH_URL1      "^((file|https?|ftps?)://(" USER "@)?)[" HOSTCHARS ".]+(:[0-9]+)?" \
                        "(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#%]*[^]'.}>) \t\r\n,\\\"])?$"
#define MATCH_URL2      "^(www|ftp)[" HOSTCHARS "]*\\.[" HOSTCHARS ".]+(:[0-9]+)?" \
                        "(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#%]*[^]'.}>) \t\r\n,\\\"])?$"
#define MATCH_EMAIL     "^(mailto:)?[a-z0-9][a-z0-9.-]*@[a-z0-9][a-z0-9-]*(\\.[a-z0-9][a-z0-9-]*)+$"

/*********************************************************************
 *
 * Initialize/shutdown Verve
 * -------------------------
 *
 * The execution of these functions is necessary before and after 
 * your program makes use of Verve.
 *
 *********************************************************************/

void 
verve_init (void)
{
  /* Init history database */
  _verve_history_init ();
}

void
verve_shutdown (void)
{
  /* Free history database */
  _verve_history_shutdown ();

  /* Shutdown command db */
  _verve_db_shutdown ();
  
  /* Shutdown environment */
  _verve_env_shutdown ();
}

/*********************************************************************
 *
 * Verve command line execution function
 * -------------------------------------
 *
 * With the help of this function, shell commands can be executed.
 *
 *********************************************************************/
 
gboolean verve_spawn_command_line (const gchar *cmdline)
{
  gint argc;
  gchar **argv;
  gboolean success;
  GError *error = NULL;
  
  success = g_shell_parse_argv (cmdline, &argc, &argv, &error);
  
  const gchar *home_dir = xfce_get_homedir ();
  GSpawnFlags flags = G_SPAWN_STDOUT_TO_DEV_NULL;
  flags |= G_SPAWN_STDERR_TO_DEV_NULL;
  flags |= G_SPAWN_SEARCH_PATH;
  
  success = g_spawn_async (home_dir, argv, NULL, flags, NULL, NULL, NULL, &error);
  
  g_strfreev (argv);
  
  return success;
}

/*********************************************************************
 * 
 * Verve main execution method
 * ---------------------------
 *
 * This method should be used whenever you want to run command line 
 * input, be it a URL, an eMail address or a custom command.
 *
 *********************************************************************/

gboolean
verve_execute (const gchar *input)
{
  VerveDb *db = verve_db_get ();

  if (_verve_is_url (input) || _verve_is_email (input))
  {
    gchar *command = g_strconcat ("exo-open ", input, NULL);
    verve_spawn_command_line (command);
    g_free (command);
    return TRUE;
  }
  else if (verve_db_has_command (db, input))
  {
    return verve_db_exec_command (db, input);
  }
  else
  {
    if (verve_spawn_command_line (input))
      return TRUE;
    else
      return FALSE;
  }
}

/*********************************************************************
 *
 * Internal pattern matching functions
 *
 *********************************************************************/

gboolean
_verve_is_url (const gchar *str)
{
  GString *string = g_string_new (str);
  const gchar *error;
  int error_offset;
  int count;
  int ovector[30];
  pcre *pattern;

  gboolean success = FALSE;

  /* Check first pattern */
  pattern = pcre_compile (MATCH_URL1, 0, &error, &error_offset, NULL);
  if (pcre_exec (pattern, NULL, string->str, string->len, 0, 0, ovector, 30) >= 0)
    success = TRUE;

  pcre_free (pattern);

  if (success)
  {
    g_string_free (string, TRUE);
    return TRUE;
  }

  /* Check second pattern */
  pattern = pcre_compile (MATCH_URL2, 0, &error, &error_offset, NULL);
  if (pcre_exec (pattern, NULL, string->str, string->len, 0, 0, ovector, 30) >= 0)
    success = TRUE;

  pcre_free (pattern);
  g_string_free (string, TRUE);

  return success;
}

gboolean
_verve_is_email (const gchar *str)
{
  GString *string = g_string_new (str);
  const gchar *error;
  int error_offset;
  int count;
  int ovector[30];
  pcre *pattern;

  gboolean success = FALSE;

  pattern = pcre_compile (MATCH_EMAIL, 0, &error, &error_offset, NULL);
  if (pcre_exec (pattern, NULL, string->str, string->len, 0, 0, ovector, 30) >= 0)
    success = TRUE;

  pcre_free (pattern);
  g_string_free (string, TRUE);

  return success;
}

/* vim:set expandtab ts=1 sw=2: */
