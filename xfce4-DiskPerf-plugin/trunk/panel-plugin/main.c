/*
 * Copyright (c) 2003, 2004 Roger Seguin <roger_seguin@msn.com>
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
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static char     _main_id[] =
    "$Id: main.c,v 1.7 2004/08/25 10:08:40 rogerms Exp $";


#define DEBUG	0

#include "config_gui.h"
#include "devperf.h"
#include "debug.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


#define PLUGIN_NAME	"DiskPerf"


 /* Some platforms do not provide busy times as separate read and write
    data, but only a single value combining both */
#if  defined(__NetBSD__)
#define	SEPARATE_BUSY_TIMES	0
#elif defined(__linux__)
#define	SEPARATE_BUSY_TIMES	1
#else
#define	SEPARATE_BUSY_TIMES	1
#endif


typedef GtkWidget *Widget_t;

typedef enum statistics_t {
    IO_TRANSFER,		/* MB transferred per second */
    BUSY_TIME			/* Percentage of time the device has been
				   busy */
} statistics_t;

enum {
    /* Monitor bar data */
    R_DATA,
    W_DATA,
    RW_DATA,
    NMONITORS
};

typedef enum monitor_bar_order_t {
    RW_ORDER,
    WR_ORDER
} monitor_bar_order_t;

typedef struct param_t {
    /* Configurable parameters */
    char            acDevice[64];
#if  !defined(__NetBSD__)
    dev_t           st_rdev;
#endif
    int             fTitleDisplayed;
    char            acTitle[16];
    enum statistics_t
                    eStatistics;
    enum monitor_bar_order_t
                    eMonitorBarOrder;
    int             iMaxXferMBperSec;
    int             fRW_DataCombined;
    uint32_t        iPeriod_ms;
    GdkColor        aoColor[NMONITORS];
} param_t;

typedef struct color_selector_t {
    Widget_t        wPB;
    Widget_t        wDA;
} color_selector_t;

typedef struct conf_t {
    Widget_t        wTopLevel;
    struct gui_t    oGUI;	/* Configuration/option dialog */
    struct color_selector_t
                    aoColorWidgets[NMONITORS];	/* Color selection dialog */
    struct param_t  oParam;
} conf_t;

typedef struct perfbar_t {
    /* Individual monitor bar */
    Widget_t       *pwBar;	/* Link to monitor_t:awProgressBar */
} perfbar_t;

typedef struct monitor_t {
    /* Plugin monitor bars */
    Widget_t        wEventBox;
    Widget_t        wBox;
    Widget_t        wTitle;
    Widget_t        awProgressBar[2];	/* Physical (widget) bars */
    struct perfbar_t
                    aoPerfBar[NMONITORS];	/* Virtual bars */
    struct devperf_t
                    oPrevPerf;
} monitor_t;

typedef struct plugin_t {
    unsigned int    iTimerId;	/* Cyclic update */
    struct conf_t   oConf;
    struct monitor_t
                    oMonitor;
} plugin_t;

	/**************************************************************/

static int DisplayPerf (struct plugin_t *p_poPlugin)
 /* Get the last disk perfomance data, compute the statistics and update
    the panel-docked monitor bars */
{
    static GtkTooltips *s_poToolTips = 0;

    struct devperf_t oPerf;
    struct param_t *poConf = &(p_poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(p_poPlugin->oMonitor);
    struct perfbar_t *poPerf = poMonitor->aoPerfBar;
    uint64_t        iInterval_ns, rbytes, wbytes, iRBusy_ns, iWBusy_ns;
    const double    K = 1.0 * 1000 * 1000 * 1000 / 1024 / 1024;
    /* bytes/ns --> MB/s */
    double          arPerf[NMONITORS], arBusy[NMONITORS], *prData, *pr;
    char            acToolTips[128];
    int             status, i;

    if (!s_poToolTips)
	s_poToolTips = gtk_tooltips_new ();

    memset (&oPerf, 0, sizeof (oPerf));
    oPerf.qlen = -1;
#if defined (__NetBSD__)
    status = DevGetPerfData (poConf->acDevice, &oPerf);
#else
    status = DevGetPerfData (&(poConf->st_rdev), &oPerf);
#endif
    if (status == -1)
	return (-1);
    if (poMonitor->oPrevPerf.timestamp_ns) {
	iInterval_ns =
	    oPerf.timestamp_ns - poMonitor->oPrevPerf.timestamp_ns;
	rbytes = oPerf.rbytes - poMonitor->oPrevPerf.rbytes;
	wbytes = oPerf.wbytes - poMonitor->oPrevPerf.wbytes;
	iRBusy_ns = oPerf.rbusy_ns - poMonitor->oPrevPerf.rbusy_ns;
	iWBusy_ns = oPerf.wbusy_ns - poMonitor->oPrevPerf.wbusy_ns;
    }
    else
	iInterval_ns = 0;
    poMonitor->oPrevPerf = oPerf;
    if (!iInterval_ns)
	return (1);

    arPerf[R_DATA] = K * rbytes / iInterval_ns;
    arPerf[W_DATA] = K * wbytes / iInterval_ns;
    arPerf[RW_DATA] = K * (rbytes + wbytes) / iInterval_ns;

    if (oPerf.qlen < 0)
	for (i = 0; i < NMONITORS; i++)
	    arBusy[i] = 0;
    else {
	arBusy[R_DATA] = (double) 100.0 *iRBusy_ns / iInterval_ns;
	arBusy[W_DATA] = (double) 100.0 *iWBusy_ns / iInterval_ns;
	arBusy[RW_DATA] =
	    (double) 100.0 *(iRBusy_ns + iWBusy_ns) / iInterval_ns;
	for (i = 0; i < NMONITORS; i++) {
	    pr = arBusy + i;
	    if (*pr > 100)
		*pr = 100;
	}
    }

    sprintf (acToolTips, "%s\n"
	     "----------------\n"
	     "I/O    (MB/s)\n"
	     "  Read :%3u\n"
	     "  Write :%3u\n"
	     "  Total :%3u\n"
	     "Busy time (%c)\n"
	     "  Read : %3d\n"
	     "  Write : %3d\n"
	     "  Total : %3d",
	     poConf->acTitle,
	     (unsigned int) (arPerf[R_DATA] + 0.5),
	     (unsigned int) (arPerf[W_DATA] + 0.5),
	     (unsigned int) (arPerf[RW_DATA] + 0.5),
	     '%',
	     SEPARATE_BUSY_TIMES && (oPerf.qlen >= 0) ?
	     (int) (arBusy[R_DATA] + 0.5) : -1,
	     SEPARATE_BUSY_TIMES && (oPerf.qlen >= 0) ?
	     (int) (arBusy[W_DATA] + 0.5) : -1,
	     (oPerf.qlen >= 0) ? (int) (arBusy[RW_DATA] + 0.5) : -1);
    gtk_tooltips_set_tip (s_poToolTips, GTK_WIDGET (poMonitor->wEventBox),
			  acToolTips, 0);

    switch (poConf->eStatistics) {
	case BUSY_TIME:
	    prData = arBusy;
	    for (i = 0; i < NMONITORS; i++)
		prData[i] /= 100;
	    break;
	case IO_TRANSFER:
	default:
	    prData = arPerf;
	    for (i = 0; i < NMONITORS; i++)
		prData[i] /= poConf->iMaxXferMBperSec;
	    break;
    }
    for (i = 0; i < NMONITORS; i++) {
	pr = prData + i;
	if (*pr > 1)
	    *pr = 1;
	else if (*pr < 0)
	    *pr = 0;
    }
    if (poConf->fRW_DataCombined)
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR
				       (*(poPerf[RW_DATA].pwBar)),
				       prData[RW_DATA]);
    else {
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR
				       (*(poPerf[R_DATA].pwBar)),
				       prData[R_DATA]);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR
				       (*(poPerf[W_DATA].pwBar)),
				       prData[W_DATA]);
    }

    return (0);
}				/* DisplayPerf() */

	/**************************************************************/

static gboolean SetTimer (void *p_pvPlugin)
	/* Recurrently update the panel-docked monitor bars through a
	   timer */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);

    if (poPlugin->iTimerId) {
	g_source_remove (poPlugin->iTimerId);
	poPlugin->iTimerId = 0;
    }
    XFCE_PANEL_LOCK ();
    DisplayPerf (poPlugin);
    XFCE_PANEL_UNLOCK ();
    poPlugin->iTimerId = g_timeout_add (poConf->iPeriod_ms,
					(GtkFunction) SetTimer, poPlugin);
    return (1);
}				/* SetTimer() */

	/**************************************************************/

static int SetSingleBarColor (struct plugin_t *p_poPlugin, int p_iBar)
	/* Set the color of a single monitor bar */
{
    struct plugin_t *poPlugin = p_poPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);
    GtkRcStyle     *poStyle;
    Widget_t       *pwBar;

    pwBar = poMonitor->aoPerfBar[p_iBar].pwBar;
    poStyle = gtk_widget_get_modifier_style (GTK_WIDGET (*pwBar));
    if (!poStyle)
	poStyle = gtk_rc_style_new ();
    else {
	poStyle->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
	poStyle->bg[GTK_STATE_PRELIGHT] = poConf->aoColor[p_iBar];
    }
    gtk_widget_modify_style (GTK_WIDGET (*pwBar), poStyle);
    return (0);
}				/* SetSingleBarColor() */

	/**************************************************************/

static int SetMonitorBarColor (struct plugin_t *p_poPlugin)
	/* Set the monitor bar colors */
{
    struct plugin_t *poPlugin = p_poPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);

    if (poConf->fRW_DataCombined)
	SetSingleBarColor (p_poPlugin, RW_DATA);
    else {
	SetSingleBarColor (p_poPlugin, R_DATA);
	SetSingleBarColor (p_poPlugin, W_DATA);
    }
    return (0);
}				/* SetMonitorBarColor() */

	/**************************************************************/

static int ResetMonitorBar (struct plugin_t *p_poPlugin)
	/* Set order (Read-Write or Write-Read) and colors */
{
    struct plugin_t *poPlugin = p_poPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);

    poMonitor->aoPerfBar[R_DATA].pwBar =
	poMonitor->awProgressBar + (poConf->eMonitorBarOrder == WR_ORDER);
    poMonitor->aoPerfBar[W_DATA].pwBar =
	poMonitor->awProgressBar + (poConf->eMonitorBarOrder == RW_ORDER);
    poMonitor->aoPerfBar[RW_DATA].pwBar = poMonitor->awProgressBar + 0;

    SetMonitorBarColor (poPlugin);

    return (0);
}				/* ResetMonitorBar() */

	/**************************************************************/

static int CreateMonitorBars (struct plugin_t *p_poPlugin,
			      int p_iOrientation)
	/* Create the panel progressive bars */
{
    struct plugin_t *poPlugin = p_poPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);
    Widget_t       *pwBar;
    int             i;

    if (p_iOrientation == HORIZONTAL)
	poMonitor->wBox = gtk_hbox_new (FALSE, 0);
    else
	poMonitor->wBox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (poMonitor->wBox);
    gtk_container_set_border_width (GTK_CONTAINER
				    (poMonitor->wBox), border_width);

    gtk_container_add (GTK_CONTAINER (poMonitor->wEventBox),
		       poMonitor->wBox);

    poMonitor->wTitle = gtk_label_new (poConf->acTitle);
    if (poConf->fTitleDisplayed)
	gtk_widget_show (poMonitor->wTitle);
    gtk_box_pack_start (GTK_BOX (poMonitor->wBox),
			GTK_WIDGET (poMonitor->wTitle), FALSE, FALSE, 0);

    for (i = 0; i < 2; i++) {
	pwBar = poMonitor->awProgressBar + i;
	*pwBar = GTK_WIDGET (gtk_progress_bar_new ());
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (*pwBar),
					  (p_iOrientation ==
					   HORIZONTAL ?
					   GTK_PROGRESS_BOTTOM_TO_TOP :
					   GTK_PROGRESS_LEFT_TO_RIGHT));
	if ((i == 1) && poConf->fRW_DataCombined)
	    gtk_widget_hide (GTK_WIDGET (*pwBar));
	else
	    gtk_widget_show (GTK_WIDGET (*pwBar));
	gtk_box_pack_start (GTK_BOX (poMonitor->wBox),
			    GTK_WIDGET (*pwBar), FALSE, FALSE, 0);
    }

    ResetMonitorBar (poPlugin);

    return (0);
}				/* CreateMonitorBars() */

	/**************************************************************/

static plugin_t *NewPlugin ()
	/* New instance of the plugin (constructor) */
{
    struct plugin_t *poPlugin;
    struct param_t *poConf;
    struct monitor_t *poMonitor;
#if !defined(__NetBSD__)
    struct stat     oStat;
    int             status;
#endif

    poPlugin = g_new (plugin_t, 1);
    memset (poPlugin, 0, sizeof (plugin_t));
    poConf = &(poPlugin->oConf.oParam);
    poMonitor = &(poPlugin->oMonitor);

#if defined(__NetBSD__)
    strcpy (poConf->acDevice, "wd0");
    strcpy (poConf->acTitle, "wd0");
#else
    strcpy (poConf->acDevice, "/dev/hda");
    status = stat (poConf->acDevice, &oStat);
    poConf->st_rdev = (status == -1 ? 0 : oStat.st_rdev);
    strcpy (poConf->acTitle, "hda");
#endif

    poConf->fTitleDisplayed = 1;

    gdk_color_parse ("#0000FF", poConf->aoColor + R_DATA);
    gdk_color_parse ("#FF0000", poConf->aoColor + W_DATA);
    gdk_color_parse ("#00FF00", poConf->aoColor + RW_DATA);

    poConf->iMaxXferMBperSec = 40;
    poConf->fRW_DataCombined = 1;
    poConf->iPeriod_ms = 500;
    poConf->eStatistics = IO_TRANSFER;
    poConf->eMonitorBarOrder = RW_ORDER;
    poPlugin->iTimerId = 0;
    poPlugin->oMonitor.oPrevPerf.timestamp_ns = 0;

    poMonitor->wEventBox = gtk_event_box_new ();
    gtk_widget_show (poMonitor->wEventBox);

    return (poPlugin);
}				/* NewPlugin() */

	/**************************************************************/

static gboolean plugin_create_control (Control * p_poCtrl)
	/* Plugin API */
	/* Create the plugin */
{
    struct plugin_t *poPlugin;

    TRACE ("plugin_create_control()\n");
    poPlugin = NewPlugin ();
    gtk_container_add (GTK_CONTAINER (p_poCtrl->base),
		       poPlugin->oMonitor.wEventBox);
    p_poCtrl->data = (gpointer) poPlugin;
    p_poCtrl->with_popup = FALSE;
    gtk_widget_set_size_request (p_poCtrl->base, -1, -1);
    return (TRUE);
}				/* plugin_create_control() */

	/**************************************************************/

static void plugin_free (Control * ctrl)
	/* Plugin API */
{
    struct plugin_t *poPlugin;

    TRACE ("plugin_free()\n");
    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);
    poPlugin = (plugin_t *) ctrl->data;
    if (poPlugin->iTimerId)
	g_source_remove (poPlugin->iTimerId);
    g_free (poPlugin);
}				/* plugin_free() */

	/**************************************************************/

	/* Configuration Keywords */
#define CONF_USE_LABEL		"UseLabel"
#define CONF_LABEL_TEXT		"Text"
#define CONF_DEVICE		"Device"
#define CONF_UPDATE_PERIOD	"UpdatePeriod"
#define CONF_STATISTICS		"Statistics"
#define CONF_XFER_RATE		"XferRate"
#define CONF_COMBINE_RW_DATA	"CombineRWdata"
#define CONF_MONITOR_BAR_ORDER	"MonitorBarOrder"
#define CONF_READ_COLOR		"ReadColor"
#define CONF_WRITE_COLOR	"WriteColor"
#define CONF_READ_WRITE_COLOR	"ReadWriteColor"

	/**************************************************************/

static void plugin_read_config (Control * p_poCtrl, xmlNodePtr p_poParent)
	/* Plugin API */
	/* Executed when the panel is started - Read the configuration
	   previously stored in xml file */
{
    struct plugin_t *poPlugin = (plugin_t *) p_poCtrl->data;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);
    Widget_t       *pw2ndBar = poPlugin->oMonitor.awProgressBar + 1;
    xmlNodePtr      poNode;
    char           *pc;
#if !defined(__NetBSD__)
    struct stat     oStat;
    int             status;
#endif

    TRACE ("plugin_read_config()\n");
    if (!p_poParent)
	return;
    for (poNode = p_poParent->children; poNode; poNode = poNode->next) {
	if (!(xmlStrEqual (poNode->name, PLUGIN_NAME)))
	    continue;

	if ((pc = xmlGetProp (poNode, (CONF_DEVICE)))) {
	    memset (poConf->acDevice, 0, sizeof (poConf->acDevice));
	    strncpy (poConf->acDevice, pc, sizeof (poConf->acDevice) - 1);
	    xmlFree (pc);
#if !defined(__NetBSD__)
	    status = stat (poConf->acDevice, &oStat);
	    poConf->st_rdev = (status == -1 ? 0 : oStat.st_rdev);
#endif
	}

	if ((pc = xmlGetProp (poNode, (CONF_USE_LABEL)))) {
	    poConf->fTitleDisplayed = atoi (pc);
	    xmlFree (pc);
	}
	if (poConf->fTitleDisplayed)
	    gtk_widget_show (GTK_WIDGET (poMonitor->wTitle));
	else
	    gtk_widget_hide (GTK_WIDGET (poMonitor->wTitle));

	if ((pc = xmlGetProp (poNode, (CONF_LABEL_TEXT)))) {
	    memset (poConf->acTitle, 0, sizeof (poConf->acTitle));
	    strncpy (poConf->acTitle, pc, sizeof (poConf->acTitle) - 1);
	    xmlFree (pc);
	    gtk_label_set_text (GTK_LABEL (poMonitor->wTitle),
				poConf->acTitle);
	}

	if ((pc = xmlGetProp (poNode, (CONF_UPDATE_PERIOD)))) {
	    poConf->iPeriod_ms = atoi (pc);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_STATISTICS)))) {
	    poConf->eStatistics = atoi (pc);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_XFER_RATE)))) {
	    poConf->iMaxXferMBperSec = atoi (pc);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_COMBINE_RW_DATA)))) {
	    poConf->fRW_DataCombined = atoi (pc);
	    xmlFree (pc);
	}
	if (poConf->fRW_DataCombined)
	    gtk_widget_hide (GTK_WIDGET (*pw2ndBar));
	else
	    gtk_widget_show (GTK_WIDGET (*pw2ndBar));

	if ((pc = xmlGetProp (poNode, (CONF_MONITOR_BAR_ORDER)))) {
	    poConf->eMonitorBarOrder = atoi (pc);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_READ_COLOR)))) {
	    gdk_color_parse (pc, poConf->aoColor + R_DATA);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_WRITE_COLOR)))) {
	    gdk_color_parse (pc, poConf->aoColor + W_DATA);
	    xmlFree (pc);
	}

	if ((pc = xmlGetProp (poNode, (CONF_READ_WRITE_COLOR)))) {
	    gdk_color_parse (pc, poConf->aoColor + RW_DATA);
	    xmlFree (pc);
	}

	ResetMonitorBar (poPlugin);
    }
    SetTimer (poPlugin);
}				/* plugin_read_config() */

	/**************************************************************/

static void plugin_write_config (Control * p_poCtrl, xmlNodePtr p_poParent)
	/* Plugin API */
	/* Write plugin configuration into xml file */
{
    struct plugin_t *poPlugin = (plugin_t *) p_poCtrl->data;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    const char     *acColorFormat = "#%02X%02X%02X";
    xmlNodePtr      poRoot;
    GdkColor       *poColor;
    char            acBuffer[16];

    TRACE ("plugin_write_config()\n");

    poRoot = xmlNewTextChild (p_poParent, NULL, PLUGIN_NAME, NULL);

    xmlSetProp (poRoot, CONF_DEVICE, poConf->acDevice);

    sprintf (acBuffer, "%d", poConf->fTitleDisplayed);
    xmlSetProp (poRoot, CONF_USE_LABEL, acBuffer);

    xmlSetProp (poRoot, CONF_LABEL_TEXT, poConf->acTitle);

    sprintf (acBuffer, "%d", poConf->iPeriod_ms);
    xmlSetProp (poRoot, CONF_UPDATE_PERIOD, acBuffer);

    sprintf (acBuffer, "%d", poConf->eStatistics);
    xmlSetProp (poRoot, CONF_STATISTICS, acBuffer);

    sprintf (acBuffer, "%d", poConf->iMaxXferMBperSec);
    xmlSetProp (poRoot, CONF_XFER_RATE, acBuffer);

    sprintf (acBuffer, "%d", poConf->fRW_DataCombined);
    xmlSetProp (poRoot, CONF_COMBINE_RW_DATA, acBuffer);

    sprintf (acBuffer, "%d", poConf->eMonitorBarOrder);
    xmlSetProp (poRoot, CONF_MONITOR_BAR_ORDER, acBuffer);

    poColor = poConf->aoColor + R_DATA;
    sprintf (acBuffer, acColorFormat,
	     poColor->red >> 8, poColor->green >> 8, poColor->blue >> 8);
    xmlSetProp (poRoot, CONF_READ_COLOR, acBuffer);

    poColor = poConf->aoColor + W_DATA;
    sprintf (acBuffer, acColorFormat,
	     poColor->red >> 8, poColor->green >> 8, poColor->blue >> 8);
    xmlSetProp (poRoot, CONF_WRITE_COLOR, acBuffer);

    poColor = poConf->aoColor + RW_DATA;
    sprintf (acBuffer, acColorFormat,
	     poColor->red >> 8, poColor->green >> 8, poColor->blue >> 8);
    xmlSetProp (poRoot, CONF_READ_WRITE_COLOR, acBuffer);
}				/* plugin_write_config() */

	/**************************************************************/

static void SetDevice (Widget_t p_wTF, void *p_pvPlugin)
	/* GUI callback setting the device name, whose performance will be 
	   displayed using the panel-docked monitor bars */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    const char     *pcDevice = gtk_entry_get_text (GTK_ENTRY (p_wTF));
#if !defined(__NetBSD__)
    struct stat     oStat;
    int             status;

    status = stat (pcDevice, &oStat);
    if (status == -1) {
	xfce_err ("%s\n"
		  "%s: %s (%d)",
		  PLUGIN_NAME, pcDevice, strerror (errno), errno);
	return;
    }
    poConf->st_rdev = oStat.st_rdev;
#endif
    memset (poConf->acDevice, 0, sizeof (poConf->acDevice));
    strncpy (poConf->acDevice, pcDevice, sizeof (poConf->acDevice) - 1);
}				/* SetDevice() */

	/**************************************************************/

static void ToggleTitle (Widget_t p_w, void *p_pvPlugin)
	/* GUI callback turning on/off the monitor bar legend */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct gui_t   *poGUI = &(poPlugin->oConf.oGUI);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);

    poConf->fTitleDisplayed =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (p_w));
    gtk_widget_set_sensitive (GTK_WIDGET (poGUI->wTF_Title),
			      poConf->fTitleDisplayed);
    if (poConf->fTitleDisplayed)
	gtk_widget_show (GTK_WIDGET (poMonitor->wTitle));
    else
	gtk_widget_hide (GTK_WIDGET (poMonitor->wTitle));
}				/* ToggleTitle() */

	/**************************************************************/

static void SetLabel (Widget_t p_wTF, void *p_pvPlugin)
	/* GUI callback setting the legend of the monitor bars */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);
    const char     *acTitle = gtk_entry_get_text (GTK_ENTRY (p_wTF));

    memset (poConf->acTitle, 0, sizeof (poConf->acTitle));
    strncpy (poConf->acTitle, acTitle, sizeof (poConf->acTitle) - 1);
    gtk_label_set_text (GTK_LABEL (poMonitor->wTitle), poConf->acTitle);
}				/* SetLabel() */

	/**************************************************************/

static void ToggleStatistics (Widget_t p_w, void *p_pvPlugin)
	/* GUI callback allowing to choose statistics to monitor */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct gui_t   *poGUI = &(poPlugin->oConf.oGUI);

    poConf->eStatistics = !(poConf->eStatistics);
    TRACE ("ToggleStatistics(): %d\n", poConf->eStatistics);
    switch (poConf->eStatistics) {
	case BUSY_TIME:
	    gtk_widget_hide (GTK_WIDGET (poGUI->wHBox_MaxIO));
	    if (!SEPARATE_BUSY_TIMES) {
		poConf->fRW_DataCombined = 1;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					      (poGUI->wTB_RWcombined),
					      poConf->fRW_DataCombined);
	    }
	    break;
	case IO_TRANSFER:
	default:
	    gtk_widget_show (GTK_WIDGET (poGUI->wHBox_MaxIO));
    }
    gtk_widget_set_sensitive (GTK_WIDGET (poGUI->wTB_RWcombined),
			      (poConf->eStatistics != BUSY_TIME)
			      || SEPARATE_BUSY_TIMES);
}				/* ToggleStatistics() */

	/**************************************************************/

static void ToggleRWintegration (Widget_t p_w, void *p_pvPlugin)
	/* GUI callback allowing either to combine write + read data in a
	   single monitor bar, or to keep them separate using 2 dedicated
	   monitor bars */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct gui_t   *poGUI = &(poPlugin->oConf.oGUI);
    Widget_t       *pw2ndBar = poPlugin->oMonitor.awProgressBar + 1;

    poConf->fRW_DataCombined =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (p_w));
    TRACE ("ToggleRWintegration(): %d\n", poConf->fRW_DataCombined);
    if (poConf->fRW_DataCombined) {
	gtk_widget_hide (GTK_WIDGET (poGUI->wTa_DualBars));
	gtk_widget_show (GTK_WIDGET (poGUI->wTa_SingleBar));
	gtk_widget_hide (GTK_WIDGET (*pw2ndBar));
    }
    else {
	gtk_widget_hide (GTK_WIDGET (poGUI->wTa_SingleBar));
	gtk_widget_show (GTK_WIDGET (poGUI->wTa_DualBars));
	gtk_widget_show (GTK_WIDGET (*pw2ndBar));
    }
    SetMonitorBarColor (poPlugin);
}				/* ToggleRWintegration() */

	/**************************************************************/

static void ToggleRWorder (Widget_t p_w, void *p_pvPlugin)
	/* GUI callback allowing to swap Read/Write monitor bars */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);

    poConf->eMonitorBarOrder = !(poConf->eMonitorBarOrder);
    TRACE ("ToggleRWorder(): %d\n", poConf->eMonitorBarOrder);
    ResetMonitorBar (poPlugin);
}				/* ToggleRWorder() */

	/**************************************************************/

static void SetXferRate (Widget_t p_wTF, void *p_pvPlugin)
	/* GUI callback setting the maximum I/O transfer rate of the
	   device */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    const char     *pcXferRate = gtk_entry_get_text (GTK_ENTRY (p_wTF));
    float           r;

    poConf->iMaxXferMBperSec = atoi (pcXferRate);

    /* Make it a multiple of 5 MB/s */
    r = (double) poConf->iMaxXferMBperSec / 5;
    poConf->iMaxXferMBperSec = 5 * (int) (r + 0.5);
    if (poConf->iMaxXferMBperSec > 995)
	poConf->iMaxXferMBperSec = 995;
}				/* SetXferRate() */

	/**************************************************************/

static void SetPeriod (Widget_t p_wSc, void *p_pvPlugin)
	/* Set the update period - To be used by the timer */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    float           r;

    TRACE ("SetPeriod()\n");
    r = gtk_spin_button_get_value (GTK_SPIN_BUTTON (p_wSc));
    poConf->iPeriod_ms = (r * 1000) + 0.5;	/* rounded */

    /* Make it a multiple of 50 ms */
    r = (double) poConf->iPeriod_ms / 50;
    poConf->iPeriod_ms = 50 * (int) (r + 0.5);

}				/* SetPeriod() */

	/**************************************************************/

static void ChooseColor (Widget_t p_wPB, void *p_pvPlugin)
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct gui_t   *poGUI = &(poPlugin->oConf.oGUI);
    Widget_t        wDialog;
    GdkColor       *poColor;
    GtkColorSelection *colorsel;
    int             iPerfBar;
    int             iResponse;

    if (p_wPB == poGUI->wPB_Rcolor)
	iPerfBar = R_DATA;
    else if (p_wPB == poGUI->wPB_Wcolor)
	iPerfBar = W_DATA;
    else if (p_wPB == poGUI->wPB_RWcolor)
	iPerfBar = RW_DATA;
    else
	return;
    poColor = poConf->aoColor + iPerfBar;

    wDialog = gtk_color_selection_dialog_new (_("Select color"));
    gtk_window_set_transient_for (GTK_WINDOW (wDialog),
				  GTK_WINDOW (poPlugin->oConf.wTopLevel));
    colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (wDialog)->
				    colorsel);
    gtk_color_selection_set_previous_color (colorsel, poColor);
    gtk_color_selection_set_current_color (colorsel, poColor);
    gtk_color_selection_set_has_palette (colorsel, TRUE);

    iResponse = gtk_dialog_run (GTK_DIALOG (wDialog));
    if (iResponse == GTK_RESPONSE_OK) {
	gtk_color_selection_get_current_color (colorsel, poColor);
	gtk_widget_modify_bg (poPlugin->oConf.aoColorWidgets[iPerfBar].wDA,
			      GTK_STATE_NORMAL, poColor);
	SetMonitorBarColor (poPlugin);
    }

    gtk_widget_destroy (wDialog);
}				/* ChooseColor() */

	/**************************************************************/

static void UpdateConf (Widget_t w, void *p_pvPlugin)
	/* Called back when the configuration/options window is closed */
{
    struct plugin_t *poPlugin = (plugin_t *) p_pvPlugin;
    struct conf_t  *poConf = &(poPlugin->oConf);
    struct gui_t   *poGUI = &(poConf->oGUI);

    TRACE ("UpdateConf()\n");
    SetDevice (poGUI->wTF_Device, poPlugin);
    SetLabel (poGUI->wTF_Title, poPlugin);
    SetXferRate (poGUI->wTF_MaxXfer, poPlugin);
    SetTimer (poPlugin);
}				/* UpdateConf() */

	/**************************************************************/

static int CheckStatsAvailability ()
	/* Check disk performance statistics availability */
	/* Return 0 on success, -1 otherwise */
{
    const char     *pcStatFile = 0;
    int             status;

    status = DevCheckStatAvailability (&pcStatFile);
    if (!status)
	return (0);
    if (status < 0) {
	status *= -1;
	xfce_err ("%s\n"
		  "%s: %s (%d)\n\n"
		  "This monitor will not work!\n"
		  "Please remove it.",
		  PLUGIN_NAME, (pcStatFile ? pcStatFile : ""),
		  strerror (status), status);
	return (-1);
    }
    switch (status) {
	case NO_EXTENDED_STATS:
	    xfce_err ("%s: No disk extended statistics found!\n"
		      "Either old kernel (< 2.4.20) or not\n"
		      "compiled with CONFIG_BLK_STATS turned on.\n\n"
		      "This monitor will not work!\n"
		      "Please remove it.", PLUGIN_NAME);
	    break;
	default:
	    xfce_err ("%s: Unknown error\n\n"
		      "This monitor will not work!\n"
		      "Please remove it.", PLUGIN_NAME);
    }
    return (-1);
}				/* CheckStatsAvailability() */

	/**************************************************************/

static void About (Widget_t w, void *unused)
	/* Called back when the About button in clicked */
{
    xfce_info ("%s %s - Disk Performance Monitor\n"
	       "Display instantaneous disk I/O transfer rates and busy times "
	       "on Linux and NetBSD systems\n\n"
	       "(c) 2003, 2004 Roger Seguin <roger_seguin@msn.com>\n"
	       "NetBSD statistics collection: (c) 2003 Benedikt Meurer\n"
	       "\t<benedikt.meurer@unix-ag.uni-siegen.de>",
	       PACKAGE, VERSION);
}				/* About() */

	/**************************************************************/

static void plugin_create_options (Control * p_poCtrl,
				   GtkContainer * p_pxContainer,
				   Widget_t p_wDone)
	/* Plugin API */
	/* Create/pop up the configuration/options GUI */
{
    struct plugin_t *poPlugin = (plugin_t *) p_poCtrl->data;
    struct param_t *poConf = &(poPlugin->oConf.oParam);
    struct gui_t   *poGUI = &(poPlugin->oConf.oGUI);
    char            acBuffer[16];
    struct color_selector_t *poColorWidgets;
    Widget_t       *apwColorPB[NMONITORS];
    int             i;

    TRACE ("plugin_create_options()\n");

    (void) CheckStatsAvailability ();

    poPlugin->oConf.wTopLevel = gtk_widget_get_toplevel (p_wDone);

    (void) CreateConfigGUI (GTK_WIDGET (p_pxContainer), poGUI);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (poGUI->wTB_Title),
				  poConf->fTitleDisplayed);
    gtk_widget_set_sensitive (GTK_WIDGET (poGUI->wTF_Title),
			      poConf->fTitleDisplayed);
    g_signal_connect (GTK_WIDGET (poGUI->wTB_Title), "toggled",
		      G_CALLBACK (ToggleTitle), poPlugin);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				  (poGUI->wRB_IO),
				  (poConf->eStatistics == IO_TRANSFER));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				  (poGUI->wRB_BusyTime),
				  (poConf->eStatistics == BUSY_TIME));
    if (poConf->eStatistics == IO_TRANSFER)
	gtk_widget_show (GTK_WIDGET (poGUI->wHBox_MaxIO));
    else {
	gtk_widget_hide (GTK_WIDGET (poGUI->wHBox_MaxIO));
	if (!SEPARATE_BUSY_TIMES)
	    poConf->fRW_DataCombined = 1;
    }
    g_signal_connect (GTK_WIDGET (poGUI->wRB_IO), "toggled",
		      G_CALLBACK (ToggleStatistics), poPlugin);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				  (poGUI->wTB_RWcombined),
				  poConf->fRW_DataCombined);
    gtk_widget_set_sensitive (GTK_WIDGET (poGUI->wTB_RWcombined),
			      (poConf->eStatistics != BUSY_TIME)
			      || SEPARATE_BUSY_TIMES);
    if (poConf->fRW_DataCombined) {
	gtk_widget_hide (GTK_WIDGET (poGUI->wTa_DualBars));
	gtk_widget_show (GTK_WIDGET (poGUI->wTa_SingleBar));
    }
    else {
	gtk_widget_hide (GTK_WIDGET (poGUI->wTa_SingleBar));
	gtk_widget_show (GTK_WIDGET (poGUI->wTa_DualBars));
    }
    g_signal_connect (GTK_WIDGET (poGUI->wTB_RWcombined), "toggled",
		      G_CALLBACK (ToggleRWintegration), poPlugin);

    gtk_entry_set_text (GTK_ENTRY (poGUI->wTF_Device), poConf->acDevice);
    g_signal_connect (GTK_WIDGET (poGUI->wTF_Device), "activate",
		      G_CALLBACK (SetDevice), poPlugin);

    gtk_entry_set_text (GTK_ENTRY (poGUI->wTF_Title), poConf->acTitle);
    g_signal_connect (GTK_WIDGET (poGUI->wTF_Title), "activate",
		      G_CALLBACK (SetLabel), poPlugin);

    sprintf (acBuffer, "%d", poConf->iMaxXferMBperSec);
    gtk_entry_set_text (GTK_ENTRY (poGUI->wTF_MaxXfer), acBuffer);
    g_signal_connect (GTK_WIDGET (poGUI->wTF_MaxXfer), "activate",
		      G_CALLBACK (SetXferRate), poPlugin);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (poGUI->wSc_Period),
			       ((double) poConf->iPeriod_ms) / 1000);
    g_signal_connect (GTK_WIDGET (poGUI->wSc_Period), "value_changed",
		      G_CALLBACK (SetPeriod), poPlugin);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				  (poGUI->wRB_ReadWriteOrder),
				  (poConf->eMonitorBarOrder == RW_ORDER));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				  (poGUI->wRB_WriteReadOrder),
				  (poConf->eMonitorBarOrder == WR_ORDER));
    g_signal_connect (GTK_WIDGET (poGUI->wRB_ReadWriteOrder), "toggled",
		      G_CALLBACK (ToggleRWorder), poPlugin);

    apwColorPB[R_DATA] = &(poGUI->wPB_Rcolor);
    apwColorPB[W_DATA] = &(poGUI->wPB_Wcolor);
    apwColorPB[RW_DATA] = &(poGUI->wPB_RWcolor);
    for (i = 0; i < NMONITORS; i++) {
	poColorWidgets = poPlugin->oConf.aoColorWidgets + i;
	poColorWidgets->wDA = gtk_drawing_area_new ();
	gtk_widget_modify_bg (poColorWidgets->wDA, GTK_STATE_NORMAL,
			      poConf->aoColor + i);
	gtk_container_add (GTK_CONTAINER (*(apwColorPB[i])),
			   poColorWidgets->wDA);
	gtk_widget_show (GTK_WIDGET (poColorWidgets->wDA));
	g_signal_connect (GTK_WIDGET (*(apwColorPB[i])), "clicked",
			  G_CALLBACK (ChooseColor), poPlugin);
    }

    g_signal_connect (GTK_WIDGET (p_wDone), "clicked",
		      G_CALLBACK (UpdateConf), poPlugin);
		      
    g_signal_connect (GTK_WIDGET (poGUI->wPB_About), "clicked",
		      G_CALLBACK (About), 0);
		      
		      

}				/* plugin_create_options() */

	/**************************************************************/

static void plugin_attach_callback (Control * ctrl, const gchar * signal,
				    GCallback cb, gpointer data)
	/* Plugin API */
	/* This function has to be defined, even when not being used at
	   all */
{
    TRACE ("plugin_attach_callback()\n");
}				/* plugin_attach_callback() */

	/**************************************************************/

static void plugin_set_size (Control * p_poCtrl, int p_size)
	/* Plugin API */
	/* Set the size of the panel-docked monitor bars */
{
    struct plugin_t *poPlugin = (plugin_t *) p_poCtrl->data;
    int             size1, size2;
    int             i;
    Widget_t       *pwBar;

    TRACE ("plugin_set_size()\n");
    if (settings.orientation == HORIZONTAL)
	size1 = 6 + 2 * p_size, size2 = icon_size[p_size];
    else
	size1 = icon_size[p_size], size2 = 6 + 2 * p_size;
    for (i = 0; i < 2; i++) {
	pwBar = poPlugin->oMonitor.awProgressBar + i;
	gtk_widget_set_size_request (GTK_WIDGET (*pwBar), size1, size2);
	gtk_widget_queue_resize (GTK_WIDGET (*pwBar));
    }
}				/* plugin_set_size() */

	/**************************************************************/

static void plugin_set_orientation (Control * p_poCtrl, int p_iOrientation)
	/* Plugin API */
	/* Invoked when the panel changes orientation */
{
    struct plugin_t *poPlugin = (plugin_t *) p_poCtrl->data;
    struct monitor_t *poMonitor = &(poPlugin->oMonitor);

    TRACE ("plugin_set_orientation()\n");
    if (poPlugin->iTimerId) {
	g_source_remove (poPlugin->iTimerId);
	poPlugin->iTimerId = 0;
    }
    gtk_container_remove (GTK_CONTAINER (poMonitor->wEventBox),
			  GTK_WIDGET (poMonitor->wBox));
    CreateMonitorBars (poPlugin, p_iOrientation);
    SetTimer (poPlugin);
}				/* plugin_set_orientation() */

	/**************************************************************/

G_MODULE_EXPORT void xfce_control_class_init (ControlClass * cc)
	/* Plugin API */
	/* Main */
{
    TRACE ("xfce_control_class_init()\n");

    (void) DevPerfInit ();

    cc->name = PLUGIN_NAME;
    cc->caption = _("Disk Performance");
    cc->create_control = (CreateControlFunc) plugin_create_control;
    cc->free = plugin_free;
    cc->attach_callback = plugin_attach_callback;
    cc->read_config = plugin_read_config;
    cc->write_config = plugin_write_config;
    cc->create_options = plugin_create_options;
    cc->set_size = plugin_set_size;
    cc->set_orientation = plugin_set_orientation;
    cc->set_theme = 0;
}				/* xfce_control_class_init() */

	/* Plugin API */
XFCE_PLUGIN_CHECK_INIT
	/**************************************************************/
/*
$Log: main.c,v $
Revision 1.7  2004/08/25 10:08:40  rogerms
DiskPerf 1.5

Revision 1.14  2004/08/25 08:51:02  RogerSeguin
MT support and About... dialog box added

Revision 1.6  2003/11/11 12:40:34  rogerms
Release 1.4

Revision 1.13  2003/11/11 12:12:42  RogerSeguin
Force to single "total busy time" monitor when platform doesn't provide separate read and write data (e.g. NetBSD)

Revision 1.5  2003/11/04 10:26:13  rogerms
DiskPerf 1.3

Revision 1.12  2003/11/04 10:16:36  RogerSeguin
Got rid of Microsoft ^M

Revision 1.11  2003/11/04 09:43:36  RogerSeguin
Added busy time monitoring for Linux

Revision 1.4  2003/11/02 06:57:50  rogerms
Release 1.2

Revision 1.10  2003/11/02 06:18:33  RogerSeguin
Added busy time in tooltips for Linux 2.4 and 2.6

Revision 1.9  2003/10/24 11:16:20  RogerSeguin
Different scalable fonts with Mandrake 9.2 ==> diffent tooltips string spacing

Revision 1.3  2003/10/18 23:02:58  rogerms
DiskPerf release 1.1

Revision 1.8  2003/10/18 06:56:50  RogerSeguin
Integration of Benedikt Meurer's work on NetBSD port

Revision 1.7  2003/10/16 13:07:42  RogerSeguin
Kernel 2.6 support

Revision 1.2  2003/10/16 18:48:39  benny
Added support for NetBSD.

Revision 1.1.1.1  2003/10/07 03:39:25  rogerms
Initial release - v1.0

Revision 1.6  2003/10/02 04:16:07  RogerSeguin
Compute using rbytes/wbytes instead of rsect/wsect

Revision 1.5  2003/09/25 12:24:11  RogerSeguin
Implemented some error processing

Revision 1.4  2003/09/25 09:32:13  RogerSeguin
Added color configuration

Revision 1.3  2003/09/24 10:56:36  RogerSeguin
Now swapping the monitor bars is possible

Revision 1.2  2003/09/23 15:17:01  RogerSeguin
Now supports panel orientation

Revision 1.1  2003/09/22 02:25:35  RogerSeguin
Initial revision

*/
