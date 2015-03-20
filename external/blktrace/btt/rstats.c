/*
 * blktrace output analysis: generate a timeline & gather statistics
 *
 * (C) Copyright 2009 Hewlett-Packard Development Company, L.P.
 *	Alan D. Brunelle (alan.brunelle@hp.com)
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "globals.h"

unsigned int calc_freq;

struct files {
	FILE *fp;
	char *nm;
};

struct rstat {
	struct list_head head;
	struct files files[6];
	unsigned long long ios, nblks;
	unsigned long long rios, wios, rblks, wblks;
	double base_msec;
};

static struct rstat *sys_info;
static LIST_HEAD(rstats);

static int do_open(struct files *fip, char *bn, char *pn)
{
	fip->nm = malloc(strlen(bn) + 16);
	sprintf(fip->nm, "%s_%s.dat", bn, pn);

	fip->fp = my_fopen(fip->nm, "w");
	if (fip->fp) {
		add_file(fip->fp, fip->nm);
		return 0;
	}

	free(fip->nm);
	return -1;
}

static int init_rsip(struct rstat *rsip, struct d_info *dip)
{
	char *nm = dip ? dip->dip_name : "sys";
	char fname[256];

	if (calc_freq <= 0 || calc_freq > 1000)
		calc_freq = 1000;

	rsip->base_msec = -1;
	rsip->ios = rsip->nblks = 0;
	rsip->rios = rsip->wios = rsip->rblks = rsip->wblks = 0;

	if (output_name)
		snprintf(fname, 255, "%s_%s", output_name, nm);
	else snprintf(fname, 255, "%s", nm);

	if (do_open(&rsip->files[0], fname, "iops") ||
		do_open(&rsip->files[1], fname, "mbps") ||
		do_open(&rsip->files[2], fname, "r_iops") ||
		do_open(&rsip->files[3], fname, "r_mbps") ||
		do_open(&rsip->files[4], fname, "w_iops") ||
		do_open(&rsip->files[5], fname, "w_mbps"))
		return -1;

	list_add_tail(&rsip->head, &rstats);
	return 0;
}

static void rstat_emit(struct rstat *rsip, double cur)
{
	double mbps, base_sec;
	double resolution = MSEC_IN_SEC / (double)calc_freq;

	/* round base down to closest multiple of frequency */
	base_sec = (unsigned int)(rsip->base_msec -
		    ((unsigned int)rsip->base_msec % calc_freq)) / 1000.0;

	/*
	 * I/Os per second is easy: just the ios, normalized
	 */
	fprintf(rsip->files[0].fp, "%.3lf %llu\n", base_sec,
		(unsigned long long)(rsip->ios * resolution));

	/*
	 * MB/s we convert blocks to mb...
	 */
	mbps = ((double)rsip->nblks * 512.0 * resolution) / (1024.0 * 1024.0);
	fprintf(rsip->files[1].fp, "%.3lf %.2lf\n", base_sec, mbps);

	/* Read/Write specific IOPS and MBPS */
	fprintf(rsip->files[2].fp, "%.3lf %llu\n", base_sec,
		rsip->rios * (unsigned long long)resolution);
	mbps = ((double)rsip->rblks * 512.0 * resolution) / (1024.0 * 1024.0);
	fprintf(rsip->files[3].fp, "%.3lf %.2lf\n", base_sec, mbps);

	fprintf(rsip->files[4].fp, "%.3lf %llu\n", base_sec,
		rsip->wios * (unsigned long long)resolution);
	mbps = ((double)rsip->wblks * 512.0 * resolution) / (1024.0 * 1024.0);
	fprintf(rsip->files[5].fp, "%.3lf %.2lf\n", base_sec, mbps);

	rsip->base_msec = TO_MSEC(cur);
	rsip->ios = rsip->nblks = 0;
	rsip->rios = rsip->rblks = 0;
	rsip->wios = rsip->wblks = 0;
}

static void __add(struct rstat *rsip, double cur, unsigned long long nblks, int rw)
{
	if (rsip->base_msec < 0)
		rsip->base_msec = TO_MSEC(cur);
	else if ((TO_MSEC(cur) - rsip->base_msec) >= (double)calc_freq)
		rstat_emit(rsip, cur);

	if (rw) {
		rsip->rios++;
		rsip->rblks += nblks;
	} else {
		rsip->wios++;
		rsip->wblks += nblks;
	}
	rsip->ios++;
	rsip->nblks += nblks;
}

void *rstat_alloc(struct d_info *dip)
{
	struct rstat *rsip = malloc(sizeof(*rsip));

	if (!init_rsip(rsip, dip))
		return rsip;

	free(rsip);
	return NULL;
}

void rstat_free(void *ptr)
{
	struct rstat *rsip = ptr;

	rstat_emit(rsip, last_t_seen);
	list_del(&rsip->head);
	free(rsip);
}

void rstat_add(void *ptr, double cur, unsigned long long nblks, int rw)
{
	if (ptr != NULL)
		__add((struct rstat *)ptr, cur, nblks, rw);
	if (sys_info != NULL)
		__add(sys_info, cur, nblks, rw);
}

int rstat_init(void)
{
	sys_info = rstat_alloc(NULL);
	return sys_info != NULL;
}

void rstat_exit(void)
{
	struct list_head *p, *q;

	list_for_each_safe(p, q, &rstats) {
		struct rstat *rsip = list_entry(p, struct rstat, head);
		rstat_free(rsip);
	}
}
