/* $Id: unsupported.c,v 1.2 2003/08/25 21:08:58 bwalle Exp $ */

/******************************************************************************
 *
 * get_stat()
 *
 * stub function for all unsupported operating systems
 *
 *****************************************************************************/

int get_stat(netdata* data)
{
    return 0;
}

void init_osspecific(netdata* data)
{
    /* do nothgin */
}


int checkinterface(netdata* data)
{
    return 0;
}
