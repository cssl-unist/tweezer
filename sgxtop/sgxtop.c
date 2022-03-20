/* Copyright (c) Fortanix, Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/queue.h>
#include <linux/taskstats.h>

#define SGX_STATS	"/proc/sgx_stats"
#define SGX_ENCLAVES	"/proc/sgx_enclaves"

/* The number of hash buckets to create, not the limit on enclaves */
#define ENCLAVE_BUCKETS 101

FILE* log_fp = NULL;
struct stats {
	unsigned int enclaves_created;
	unsigned int enclaves_released;
	unsigned long pages_added;
	unsigned long pageins;
	unsigned long pageouts;
	unsigned int enclave_pages;
	unsigned int va_pages;
	unsigned int free_pages;
	struct timespec readtime;
};

struct enclave {
	pid_t pid;
	unsigned int id;
	unsigned long size;
	unsigned long eadd_cnt;
	unsigned long resident;
};

struct enclave_entry {
	struct enclave enclave;
	char *command; /* process command line */
	LIST_ENTRY(enclave_entry) state_entry[2];
	LIST_ENTRY(enclave_entry) hash_entry;
};


static int pid_width = 10;  /* pick a really big size and adjust down */
static bool sgxtop = true;  /* sgxtop (continuous reporting) or sgxstat
			     * (one shot) */

/*
 * Keep an old and a new  list of enclaves, and a hash table.
 * Look 'em up quickly in the hash table as we read them.
 * Put them in the new list as they are read.  Remove the old
 * list when we've read all of the new enclaves.
 */

LIST_HEAD(enclave_head, enclave_entry);

struct enclaves {
	unsigned int count;
	unsigned int state;
	struct enclave_head *hash_table;
	size_t hash_table_size;
	struct enclave_head state_list[2];
	struct timespec readtime;
};

#define NSEC_PER_SEC 1000000000

void do_init()
{
	/* Some systems allow larger PIDs than the default of 32767.
	 * Ensure that the fields look right for them. */

	FILE *fp = fopen("/proc/sys/kernel/pid_max", "ro");

	if (fp) {
		pid_width = 0;
		while (fgetc(fp) != EOF) {
			pid_width++;
		}
		if (pid_width > 0) {
			pid_width--;
		}
	}
	fclose(fp);
}

long int timespec_diff(struct timespec *later, struct timespec *earlier)
{
	long int diff = (later->tv_sec - earlier->tv_sec) * NSEC_PER_SEC;

	diff += later->tv_nsec - earlier->tv_nsec;
	assert(diff >= 0);

	return diff;
}

int sleep_til(struct timespec *when)
{
	int rc;
	while ((rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				     when, NULL)) != 0) {
		if (rc != EINTR) {
			fprintf(stderr,
				"clock_nanosleep(2) returned %d, errno %d\n",
				rc, errno);
			return rc;
		}
	}
	return rc;
}

int stats_read(struct stats *stats)
{
	FILE *fp;

	if (!(fp = fopen(SGX_STATS, "ro"))) {
		fprintf(stderr, "failed to read %s\n", SGX_STATS);
		return -1;
	}

	int r = fscanf(fp, "%u %u %lu %lu %lu %u %u %u\n",
		       &stats->enclaves_created, &stats->enclaves_released,
		       &stats->pages_added, &stats->pageins, &stats->pageouts,
		       &stats->enclave_pages, &stats->va_pages,
		       &stats->free_pages);
	fclose(fp);
	if (r != 8) {
		fprintf(stderr, "expect to read %d entries from %s, got %d\n",
			8, SGX_STATS, r);
		return -1;
	}

	r = clock_gettime(CLOCK_MONOTONIC, &stats->readtime);
	if (r) {
		fprintf(stderr, "clock_gettime(3) returned %d, errno %d\n",
			r, errno);
		return r;
	}

	return 0;
}


void stats_report(struct stats *old, struct stats *new,struct timespec* ts)
{
	char enclave_str[80];

        if (log_fp != stdout) {
            if (sgxtop)
                mvprintw(0, 0, "Enclaves running:  %3u  Total enclaves created:  %u",
                        new->enclaves_created - new->enclaves_released,
                        new->enclaves_created);
            else
                printf("Enclaves running:  %3u  Total enclaves created:  %u\n",
                        new->enclaves_created - new->enclaves_released,
                        new->enclaves_created);
        }

        snprintf(enclave_str, sizeof(enclave_str), "%uK/%uK/%uK",
                new->va_pages * 4,
                (new->enclave_pages - new->free_pages) * 4,
                new->enclave_pages * 4);
        if (log_fp != stdout) {
            if (sgxtop)
                mvprintw(2, 0, "EPC mem: %11uK free: %11uK used: %11uK VA: %11uK",
                        new->enclave_pages * 4,
                        new->free_pages * 4,
                        (new->enclave_pages - new->free_pages) * 4,
                        new->va_pages * 4);
            else
                printf("EPC mem: %11uK free: %11uK used: %11uK VA: %11uK\n\n",
                        new->enclave_pages * 4, new->free_pages * 4,
                        (new->enclave_pages - new->free_pages) * 4,
                        new->va_pages * 4);
        }
	/* sgxstats doesn't report time-depended data */
	if (!sgxtop)
		return;

	long int time_diff = timespec_diff(&new->readtime, &old->readtime);

	long unsigned pageins = new->pageins - old->pageins;
	long unsigned pageouts = new->pageouts - old->pageouts;
	if (time_diff != 0) {
		pageins = 4 * pageins * NSEC_PER_SEC / time_diff;
		pageouts = 4 * pageouts * NSEC_PER_SEC / time_diff;
	} else {
		pageins = 0;
		pageouts = 0;
	}

        if (log_fp != stdout) {
	mvprintw(1, 0,
		 "EPC paging per second,  in:  %10luK out:   %10luK",
		 pageins, pageouts);
        }
	if(log_fp != NULL && ts != NULL)
	{
		char output[128];
		snprintf(output,128,"%lu; %10lu; %10lu; %10u; %10u; %10u; %10u;\r\n",
                        ts->tv_sec, pageins, pageouts,new->enclave_pages * 4, new->free_pages * 4, (new->enclave_pages - new->free_pages) * 4, new->va_pages * 4 );
		fputs(output,log_fp);
	} 
        if (log_fp != stdout) {
            refresh();
        }
}

char *pid_read_command(pid_t pid)
{
	FILE *fp;
	char filename[sizeof("/proc//comm") + pid_width];
	char command[TS_COMM_LEN];
	char *result;

	snprintf(filename, sizeof(filename), "/proc/%d/comm", pid);
	fp = fopen(filename, "ro");
	if (!fp)
		return NULL;

	result = fgets(command, sizeof(command), fp);
	fclose(fp);

	if (result)
		result = strdup(result);

	return result;
}

int enclave_read(FILE *fp, struct enclave *enclave)
{
	int r = fscanf(fp, "%d %u %lu %lu %lu",
		       &enclave->pid, &enclave->id,
		       &enclave->size, &enclave->eadd_cnt,
		       &enclave->resident);
	if (r != 5)
		return -1;

	return 0;
}

void enclave_update(struct enclave_entry *o, struct enclave *n, unsigned which)
{
	assert(o->enclave.id == n->id && o->enclave.pid == n->pid);
	o->enclave = *n;
}

void enclave_delete(struct enclaves *enclaves, struct enclave_entry *e,
		    unsigned which)
{
	if (e->command) {
		free(e->command);
	}
	memset(e, 0, sizeof(struct enclave_entry));
	free(e);
}

size_t hash_func(struct enclaves *e, unsigned int id)
{
	return id % e->hash_table_size;
}

struct enclaves *enclaves_create(size_t n)
{
	struct enclaves *e = calloc(1, sizeof(struct enclaves));
	if (!e)
		return NULL;

	e->hash_table_size = n;
	e->hash_table = malloc(sizeof(struct enclave_head) * n);
	if (!e->hash_table) {
		free(e);
		return NULL;
	}

	for (int i = 0; i < n; i++)
		LIST_INIT(&e->hash_table[i]);

	e->state = 0;
	LIST_INIT(&e->state_list[0]);
	LIST_INIT(&e->state_list[1]);

	return e;
}

struct enclave_entry *enclaves_find(struct enclaves *enclaves, unsigned int id)
{
	size_t bucket = hash_func(enclaves, id);
	struct enclave_entry *e = LIST_FIRST(&enclaves->hash_table[bucket]);

	while (e) {
		if (e->enclave.id == id)
			return e;
		e = LIST_NEXT(e, hash_entry);
	}
	return NULL;
}

void enclaves_insert(struct enclaves *enclaves, struct enclave_entry *e)
{
	size_t bucket = hash_func(enclaves, e->enclave.id);
	LIST_INSERT_HEAD(&enclaves->hash_table[bucket], e, hash_entry);
}

void enclaves_check_list(struct enclaves *enclaves)
{
	struct enclave_entry *e;
	int count = 0;

	LIST_FOREACH(e, &enclaves->state_list[enclaves->state],
		     state_entry[enclaves->state]) {
		assert(++count <= enclaves->count);
	}
}

void enclaves_read(struct enclaves *enclaves)
{
	/*
	 * We track old and new entries, and have to swap which list
	 * represents new.
	 */

	unsigned old_state = enclaves->state;
	unsigned new_state = !enclaves->state;

	enclaves->state = new_state;
	assert(enclaves->state_list[new_state].lh_first == NULL);

	FILE *fp = fopen(SGX_ENCLAVES, "ro");
	if (!fp) {
		fprintf(stderr, "Couldn't open %s\n", SGX_ENCLAVES);
		exit(-1);
	}

	struct enclave enclave;
	struct enclave_entry *e;

	enclaves->count = 0;
	while (!enclave_read(fp, &enclave)) {
		enclaves->count++;

		if ((e = enclaves_find(enclaves, enclave.id)) != NULL) {
			enclave_update(e, &enclave, old_state);
			/*
			 * Since this enclave was in the hash table,
			 * it was pre-existing, and should be removed
			 * from the list of old enclaves.
			 */
			LIST_REMOVE(e, state_entry[old_state]);
		} else {
			e = calloc(1, sizeof(struct enclave_entry));
			if (!e) {
				fprintf(stderr, "malloc failed!\n");
				exit(1);
			}

			e->enclave = enclave;
			e->command = pid_read_command(e->enclave.pid);

			enclaves_insert(enclaves, e);
		}
		/* Insert everybody in the list of current enclaves */
		LIST_INSERT_HEAD(&enclaves->state_list[new_state],
				 e, state_entry[new_state]);
	}
	fclose(fp);

	int r = clock_gettime(CLOCK_MONOTONIC, &enclaves->readtime);
	if (r) {
		fprintf(stderr, "Clock failed to read!\n");
		exit(-1);
	}

	/* Iterate over the table of old enclaves and remove each one. */
	while (!LIST_EMPTY(&enclaves->state_list[old_state])) {
		e = LIST_FIRST(&enclaves->state_list[old_state]);
		LIST_REMOVE(e, state_entry[old_state]);
		LIST_REMOVE(e, hash_entry);
		enclave_delete(enclaves, e, old_state);
	}
	assert(enclaves->state_list[old_state].lh_first == NULL);
}

int enclave_compar(const void *fv, const void *sv)
{
	const struct enclave **f = (const struct enclave **) fv;
	const struct enclave **s = (const struct enclave **) sv;

	/*
	 * Simple sort by resident set size and ID;  note that we
	 * have to be careful about signed arithmetic.  And invert
	 * the check to put the most active and earliest enclaves
	 * first.
	 */
	long long int rc = (long long int) (*f)->resident -
		(long long int) (*s)->resident;

	if (rc == 0)
		rc = (long long) (*f)->id - (long long) (*s)->id;

	if (rc < 1)
		return 1;
	else if (rc > 1)
		return -1;
	else
		return 0;
}

void enclaves_report(struct enclaves *enclaves)
{
	struct enclave_entry *list[enclaves->count];
	struct enclave_entry *e;
	unsigned line = 4;
	size_t count = 0;
	static unsigned last_lines;

        if (log_fp != stdout) {
	if (sgxtop)
		mvprintw(line++, 0, "%*s %10s %11s %11s %11s %10s", pid_width,
			 "PID", "ID", "Size", "EADDs", "Resident", "Command");
	else
		printf("%*s %10s %11s %11s %11s %10s\n", pid_width,
		       "PID", "ID", "Size", "EADDs", "Resident", "Command");
        }
	LIST_FOREACH(e, &enclaves->state_list[enclaves->state],
		     state_entry[enclaves->state]) {
		assert(count < enclaves->count);
		list[count++] = e;
	}

	assert(count == enclaves->count);

	qsort(list, enclaves->count, sizeof(struct enclave_entry *),
	      enclave_compar);

	for (count = 0; count < enclaves->count;
	     count++, line++) {
		assert(line <= enclaves->count + 4 /* initial count */);
		e = list[count];
                if(log_fp != stdout) {
		if (sgxtop)
			mvprintw(line, 0, "%*d %10u %10luK %10luK %10luK %s",
				 pid_width, e->enclave.pid, e->enclave.id,
				 e->enclave.size / 1024,
				 e->enclave.eadd_cnt * 4,
				 e->enclave.resident * 4,
				 e->command ? e->command : "");
		else
			printf("%*d %10u %10luK %10luK %10luK %s",
			       pid_width, e->enclave.pid, e->enclave.id,
			       e->enclave.size / 1024, e->enclave.eadd_cnt * 4,
			       e->enclave.resident * 4,
			       e->command ? e->command : "");
                }
		if (sgxtop && line >= LINES)
			break;
	}

	if (!sgxtop)
		return;

	/* Clear any leftover lines from the last display */
	int current_lines = line;
	while (last_lines > line) {
		move(line++, 0);
		clrtoeol();
	}
        if(log_fp != stdout) {
            refresh();
        }
	last_lines = current_lines;
}


int main(int argc, char **argv)
{
	struct stats old, new;
	struct enclaves *enclaves = NULL;

	enclaves = enclaves_create(ENCLAVE_BUCKETS);
	if (!enclaves) {
		fprintf(stderr, "failed to create list of enclaves\n");
		exit(-1);
	}

	do_init();

	if (stats_read(&new)) {
		exit(-1);
	}


	/*
	 * Somewhat hackily let this executable run as either
	 * sgxtop (with continuous, top-style updates to the screen),
	 * or sgxstat (with a one-shot report).
	 */

	char *command = argv[0] + strlen(argv[0]) - 1;
	while (command > argv[0] && isalpha(*command))
		command--;
	if (command != argv[0])
		command++;

        /*
         * log stats to a file if given.
         */
        if (argc > 1) {
            if (strncmp(argv[1], "stdout", 6) == 0) {
                log_fp = stdout;
            } else {
                log_fp = fopen(argv[1],"a+");
            }
            char output[128];
            snprintf(output, 128, "timeStamp; pageIn; pageOut; Total EPC; EPCFree; EPCUsed; VA;\r\n\n");
            fputs(output, log_fp);
            fflush(log_fp);
        }


	if (strcmp(command, "sgxstat") == 0) {
		sgxtop = false;
		enclaves_read(enclaves);
		stats_report(NULL, &new,NULL);
		enclaves_report(enclaves);
		return 0;
	}

	/* Fire up curses. */

        if (log_fp != stdout) {
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	clear();
        }

	struct timespec wait = new.readtime;
	while (1) {
		old = new;
		wait.tv_sec++;
		if (sleep_til(&wait))
			exit(-1);
		if (stats_read(&new))
			exit(-1);
		enclaves_read(enclaves);
#ifdef DEBUG
		enclaves_check_list(enclaves);
#endif
		stats_report(&old, &new, &wait);
		enclaves_report(enclaves);
		/* Accept input, for instance for a redraw? */
                fflush(log_fp);
	}
        fclose(log_fp);	

	endwin();  // Not reached
	return 0;
}
