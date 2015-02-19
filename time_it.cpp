// Copyright (c) 2005-2015 Ross Smith II. See Mit LICENSE in /LICENSE

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <windows.h>

#include <stdio.h>
#include <time.h>		// time()

#include "getopt.h"

#include "version.h"

#define DEFAULT_BUCKETS		(5)
#define MAX_BUCKETS			(10)
#define	DEFAULT_KILOS		(1024)
#define	DEFAULT_FILE_SIZE	(1024 * 1024 * 100)
#define	DEFAULT_READS		(1000000)
#define	DEFAULT_SECONDS		(64)
#define	DEFAULT_LAST		(DEFAULT_SECONDS)
#define DEFAULT_REFRESH		(4) // 64 / 32 / 16 / 8 / 4

#define APPNAME			VER_INTERNAL_NAME
#define APPVERSION		VER_STRING2
#define APPCOPYRIGHT	VER_LEGAL_COPYRIGHT

static char *progname = APPNAME;

static char *short_options = "b:f:kl:m:r:s:vz:?";

static struct option long_options[] = {
  {"buckets",	required_argument,	0, 'b'},
  {"kilo",		no_argument,		0, 'k'},
  {"kilobyte",	no_argument,		0, 'k'},
  {"last",		required_argument,	0, 'l'},
  {"mode",		required_argument,	0, 'm'},
  {"read",		required_argument,	0, 'r'},
  {"refresh",	required_argument,	0, 'f'},
  {"seconds",	required_argument,	0, 's'},
  {"size",		required_argument,	0, 'z'},
  {"version",	no_argument,		0, 'v'},
  {"help",		no_argument,		0, '?'},
  {NULL,		0,					0, 0}
};

void version() {
	printf(APPNAME " " APPVERSION " - " __DATE__ "\n");
	printf(APPCOPYRIGHT "\n");
}

void _usage() {
	fprintf(stderr, "Usage: %s [options] file\n"
		"\nOptions:\n"
		" -b | --buckets n Use n buckets (default is %d, max is %d)\n"
		" -s | --seconds n Run for n seconds (default is %d)\n"
		" -l | --last    n Seconds for last bucket (default is %d)\n"
		" -m | --mode mode Mode can be 'read' or 'write' (default is read)\n"
		" -r | --read n    Read (or write) n KBs per call (default is %d)\n"
		" -z | --size n    Create a file of at most n kilobytes (default is %d)\n"
		" -f | --refresh n Refresh display every n seconds (default is %d)\n"
		" -k | --kilobyte  Use 1024 for kilobyte (default is 1000)\n"
		" -v | --version   Show version and copyright information and quit\n"
		" -? | --help      Show this help message and quit (-?? = more help, etc.)\n",
		progname,
		DEFAULT_BUCKETS,
		MAX_BUCKETS,
		DEFAULT_SECONDS,
		DEFAULT_SECONDS,
		DEFAULT_KILOS,
		DEFAULT_FILE_SIZE,
		DEFAULT_REFRESH);
}

void examples() {
	fprintf(stderr,
		"\nExamples:\n"
		" %s file                       & read from file (must already exist)\n"
		" %s --mode write file          & write to file 'file'\n"
		" %s --size 1 --mode write file & write 1KB to file 'file'\n",
		progname,
		progname,
		progname);
}

void usage(int exit_code) {
	_usage();
	exit(exit_code);
}

typedef enum {MODE_READ, MODE_WRITE} t_mode;

struct _opt {
	int	buckets;
	bool			kilobyte;
	t_mode			mode;
	int	kilos;
	int	last;
	double			refresh;
	ULONGLONG		refresh_ticks;
	int	seconds;
	ULONGLONG		size;
	int	help;
};

typedef struct _opt t_opt;

static t_opt opt = {
	DEFAULT_BUCKETS,	/* buckets */
	false,				/* kilobyte */
	MODE_READ,			/* mode */
	DEFAULT_KILOS,		/* kilos */
	DEFAULT_LAST,		/* last */
	0,					/* refresh */
	0,					/* refresh_ticks */
	DEFAULT_SECONDS,	/* seconds */
	DEFAULT_FILE_SIZE,	/* size */
	0					/* help */
};

struct _bucket {
	ULONGLONG	ticks;
	ULONGLONG	ticks_div_2;
	ULONGLONG	bytes;
	double		ticksd;
	double		mb_per_sec;
	char		*title;
	double		title_divisor;
#ifdef _DEBUG
	int			debug;
#endif
};

typedef struct _bucket t_bucket;

t_bucket **bucket;

struct _read {
	ULONGLONG		start_tick;
	ULONGLONG		end_tick;
};

typedef struct _read t_read;

t_read **read;

int reads = DEFAULT_READS;

struct _stats {
	ULONGLONG	tick_frequency;	/* ticks to seconds divisor */
	double		tick_frequencyd;
	ULONGLONG	start_ticks;
	SYSTEMTIME	lpStartTime;
	char		start_time[20];
	ULONGLONG	io_ticks;
};

typedef struct _stats t_stats;

t_stats stats;

/* per http://www.scit.wlv.ac.uk/cgi-bin/mansec?3C+basename */
static char* basename(char* s) {
	char* rv;

	if (!s || !*s)
		return ".";

	rv = s + strlen(s) - 1;

	do {
		if (*rv == '/' || *rv == '\\')
			return rv + 1;
		--rv;
	} while (rv >= s);

	return s;
}

static void FatalError(char *str) {
	LPVOID lpMsgBuf;
	DWORD err = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL);
	fprintf(stderr, "\n%s: %s\n", str, lpMsgBuf);
	exit(err ? err : 1);
}

static BOOL add_seconds(SYSTEMTIME *st, DWORD seconds, SYSTEMTIME *rv) {
	FILETIME ft;
	SystemTimeToFileTime(st, &ft);

	ULARGE_INTEGER uli;
	uli.HighPart		= ft.dwHighDateTime;
	uli.LowPart			= ft.dwLowDateTime;
	uli.QuadPart		+= (seconds * 10000000ui64);

	ft.dwHighDateTime	= uli.HighPart;
	ft.dwLowDateTime	= uli.LowPart;

	return (BOOL) FileTimeToSystemTime(&ft, rv);
};

static char *systemtime_to_hhmmss(SYSTEMTIME *st, char *rv, int bufsiz) {
	_snprintf(rv, bufsiz, "%02d:%02d:%02d", st->wHour, st->wMinute, st->wSecond);

	return rv;
}

static char *seconds_to_hhmmss(DWORD seconds, char *rv, int bufsiz) {
	DWORD hours = seconds / 3600;
	seconds -= hours * 3600;

	DWORD minutes = seconds / 60;
	seconds -= minutes * 60;

	_snprintf(rv, bufsiz, "%02d:%02d:%02d", hours, minutes, seconds);

	return rv;
}

static ULONGLONG get_tick() {
	typedef enum {STATE_UNINITIALIZED, STATE_USE_FREQUENCY, STATE_USE_TICKCOUNT} t_state;
	static t_state state = STATE_UNINITIALIZED;

	static DWORD last_ticks;
	static ULONGLONG overflow_ticks = 0;

#define USE_TICK_COUNT

	if (state == STATE_UNINITIALIZED) {
		LARGE_INTEGER frequency = {0,0};
#ifndef USE_TICK_COUNT
		QueryPerformanceFrequency(&frequency);
#endif // USE_TICK_COUNT
		if (frequency.QuadPart >= 1000) {
			state = STATE_USE_FREQUENCY;
			stats.tick_frequency = frequency.QuadPart;
		} else {
			state = STATE_USE_TICKCOUNT;
			stats.tick_frequency = 1000;
			last_ticks = GetTickCount();
		}
		stats.tick_frequencyd = (double) (LONGLONG) stats.tick_frequency;
	}

	if (state == STATE_USE_FREQUENCY) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return now.QuadPart;
	}

	DWORD ticks = GetTickCount();
	if (ticks < last_ticks) {
		overflow_ticks += 0x100000000;
	}

	return overflow_ticks + (ULONGLONG) ticks;
}

static void print_stats(int reads_made, ULONGLONG first_tick) {
	static int bytes_per_read = opt.kilos * 1024;

	static double kilosd = opt.kilobyte ? 1024 : 1000;

	int i;
	int j;

	for (i = 0; i < opt.buckets; ++i) {
		bucket[i]->bytes = 0;
	}

	ULONGLONG now_tick = get_tick();

#ifdef _DEBUG
	printf("now_tick=%d\n", now_tick);

	printf("\n");
	printf("          end   start elapsed ");
	for (i = 0; i < opt.buckets; ++i) {
		printf("%5d ", bucket[i]->ticks);
	}
	printf("\n");
	printf("    i   ticks   ticks   ticks ");
	for (i = 0; i < opt.buckets; ++i) {
		printf("%5d ", i);
	}
	printf("\n");
	printf("----- ------- ------- ------- ");
	for (i = 0; i < opt.buckets; ++i) {
		printf("----- ");
	}
	printf("\n");
#endif

	for (i = 0; i < reads_made; ++i) {
		ULONGLONG end_ticks		= now_tick - read[i]->end_tick;
		ULONGLONG start_ticks	= now_tick - read[i]->start_tick;
		ULONGLONG elapsed_ticks	= read[i]->end_tick - read[i]->start_tick;

#ifdef _DEBUG
		printf("%5d %7I64d %7I64d %7I64d ", i, end_ticks, start_ticks, elapsed_ticks);

//		printf("end_tick=%10I64d start_tick=%10I64d ", read[i]->end_tick, read[i]->start_tick);

		for (j = 0; j < opt.buckets; ++j) {
			bucket[j]->debug = 0;
		}
#endif
		for (j = 0; j < opt.buckets; ++j) {
			if (start_ticks < bucket[j]->ticks) {
#ifdef _DEBUG
				bucket[j]->debug = 1;
#endif
				bucket[j]->bytes += bytes_per_read;
				continue;
			}

			if (end_ticks >= bucket[j]->ticks) {
#ifdef _DEBUG
				bucket[j]->debug = 2;
#endif
				continue;
			}

#ifdef _DEBUG
			bucket[j]->debug = 3;

			if (elapsed_ticks) {
				ULONGLONG ull = bytes_per_read * (bucket[j]->ticks - end_ticks) / elapsed_ticks;
				if (ull > bytes_per_read) {
					__asm {int 3}
				}
			}
#endif
			if (elapsed_ticks) {
				bucket[j]->bytes += bytes_per_read * (bucket[j]->ticks - end_ticks) / elapsed_ticks;
			} else {
				bucket[j]->bytes += bytes_per_read;
			}

/*

buckets[0].ticks = 32	(0->32)
buckets[1].ticks = 16	(0->16)
buckets[2].ticks = 8	(0->8)
buckets[3].ticks = 4	(0->4)
buckets[4].ticks = 2	(0->2)

+--+----+--------+----------------+--------------------------------+
0  2    4        8                16                               32
+--+ (bucket 4)
+-------+ (bucket 3)
+----------------+ (bucket 2)
+---------------------------------+ (bucket 1)
+------------------------------------------------------------------+ (bucket 0)

32 30   28       24               16                               0
+--+----+--------+----------------+--------------------------------+
	  |        |
	  29      25

now = 32
end_tick = 29
start_tick = 25
end_ticks = 32 - 29 = 3
start_ticks = 32 - 25 = 7
elapsed_ticks = 4

*/
		}

#ifdef _DEBUG
		for (j = 0; j < opt.buckets; ++j) {
			printf("%5d ", bucket[j]->debug);
		}
		printf("\n");
#endif

	}

#ifdef _DEBUG
printf("*****************************************************************************\n");
#endif

	for (i = 0; i < opt.buckets; ++i) {
		bucket[i]->mb_per_sec = ((double) (LONGLONG) bucket[i]->bytes) / bucket[i]->ticksd / kilosd;
	}

	ULONGLONG first_ticks = now_tick - first_tick;

	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%7.3f%s", bucket[i]->mb_per_sec, sp);
	}
	printf("\r");
	printf("\n");

#ifdef _DEBUG
	printf("\n");

	printf("\nbytes: ");
	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%10I64d%s", bucket[i]->bytes, sp);
	}
	printf("\n");

	printf("\nseconds: ");
	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%05.5f%s", bucket[i]->ticksd / stats.tick_frequencyd, sp);
	}

	printf("\nbytes/sec: ");
	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%05.5f%s", ((double) (LONGLONG) bucket[i]->bytes) / bucket[i]->ticksd, sp);
	}

	printf("\nKB/sec: ");
	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%05.5f%s", ((double) (LONGLONG) bucket[i]->bytes) / bucket[i]->ticksd / kilosd, sp);
	}

	printf("\nMB/sec: ");
	for (i = 0 ; i < opt.buckets; ++i) {
		if (bucket[i]->ticks > first_ticks) {
			break;
		}
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%05.5f%s", ((double) (LONGLONG) bucket[i]->bytes) / bucket[i]->ticksd / kilosd / kilosd, sp);
	}

	printf("\n");

printf("*****************************************************************************\n");
#endif
	fflush(stdout);

}

// <adapted from truecrypt-4.2a-source-code/TrueCrypt/Common/Dlgcode.c>

static void GetSizeString (LONGLONG size, wchar_t *str) {
	static wchar_t *b, *kb, *mb, *gb, *tb, *pb;

	if (b == NULL) {
		if (opt.kilobyte) {
			kb = L"KB";
			mb = L"MB";
			gb = L"GB";
			tb = L"TB";
			pb = L"PB";
		} else {
			kb = L"KiB";
			mb = L"MiB";
			gb = L"GiB";
			tb = L"TiB";
			pb = L"PiB";
		}
		b = L"bytes";
	}

	DWORD kilo = opt.kilobyte ? 1024 : 1000;
	LONGLONG kiloI64 = kilo;
	double kilod = kilo;

	if (size > kiloI64 * kilo * kilo * kilo * kilo * 99)
		swprintf (str, L"%I64d %s", size/ kilo / kilo /kilo/kilo/kilo, pb);
	else if (size > kiloI64*kilo*kilo*kilo*kilo)
		swprintf (str, L"%.1f %s",(double)(size/kilod/kilo/kilo/kilo/kilo), pb);
	else if (size > kiloI64*kilo*kilo*kilo*99)
		swprintf (str, L"%I64d %s",size/kilo/kilo/kilo/kilo, tb);
	else if (size > kiloI64*kilo*kilo*kilo)
		swprintf (str, L"%.1f %s",(double)(size/kilod/kilo/kilo/kilo), tb);
	else if (size > kiloI64*kilo*kilo*99)
		swprintf (str, L"%I64d %s",size/kilo/kilo/kilo, gb);
	else if (size > kiloI64*kilo*kilo)
		swprintf (str, L"%.1f %s",(double)(size/kilod/kilo/kilo), gb);
	else if (size > kiloI64*kilo*99)
		swprintf (str, L"%I64d %s", size/kilo/kilo, mb);
	else if (size > kiloI64*kilo)
		swprintf (str, L"%.1f %s",(double)(size/kilod/kilo), mb);
	else if (size > kiloI64)
		swprintf (str, L"%I64d %s", size/kilo, kb);
	else
		swprintf (str, L"%I64d %s", size, b);
}

// </adapted from truecrypt-4.2a-source-code/TrueCrypt/Common/Dlgcode.c>

void print_ticks(char *fmt, ULONGLONG ticks, ULONGLONG tick_frequency) {
	char io_time[255];
	DWORD seconds = (DWORD) (ticks / tick_frequency);
	seconds_to_hhmmss(seconds, io_time, sizeof(io_time));
	printf(fmt, io_time);
}

int time_it(char *device_name) {
	stats.start_ticks = get_tick();
	stats.io_ticks = 0;
	GetLocalTime(&stats.lpStartTime);
	systemtime_to_hhmmss(&stats.lpStartTime, stats.start_time, sizeof(stats.start_time));
	stats.start_ticks = get_tick();

//	stats.device_name = device_name;

	char err[256];

	printf("File:          %s\n", device_name);

	HANDLE hnd = CreateFile(
		device_name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		opt.mode == MODE_READ ? OPEN_EXISTING : CREATE_ALWAYS, // todo fixme
		0,
		NULL);

	if (hnd == INVALID_HANDLE_VALUE) {
		_snprintf(err, sizeof(err), "Cannot open '%s'", device_name);
		FatalError(err);
	}

	ULONGLONG last_sector = 0;

	int bytes_per_read = opt.kilos * 1024;

	char *buffer = (char *) malloc(bytes_per_read);

	static double kilo = opt.kilobyte ? 1024 : 1000;
	static char * mb = opt.kilobyte ? " MB" : "MiB";

	for (int i = 0; i < opt.buckets; ++i) {
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%7.3f%s", bucket[i]->ticksd / stats.tick_frequencyd / bucket[i]->title_divisor, sp);
	}
	printf("\n");
	for (int i = 0; i < opt.buckets; ++i) {
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf(" %s%s", bucket[i]->title, sp);
	}
	printf("\n");
	for (int i = 0; i < opt.buckets; ++i) {
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("%s/sec%s", mb, sp);
	}
	printf("\n");
	for (int i = 0; i < opt.buckets; ++i) {
		char *sp = i < opt.buckets - 1 ? " " : "";
		printf("-------%s", sp);
	}
	printf("\n");

	LARGE_INTEGER li;
	DWORD lpFileSizeHigh;
	SetLastError(0);
	li.LowPart = GetFileSize(hnd, &lpFileSizeHigh);

	li.HighPart = lpFileSizeHigh;

	if (GetLastError()) {
		_snprintf(err, sizeof(err), "Failed to get file size for %s", device_name);
		FatalError(err);
	}

	ULONGLONG file_size = li.QuadPart;

	if (file_size < bytes_per_read) {
		_snprintf(err, sizeof(err), "File %s must be at least %d bytes in size", device_name, bytes_per_read);
		FatalError(err);
	}

	if (opt.size == 0) {
		opt.size = li.QuadPart;
	}

	char *action = opt.mode == MODE_READ ? "read" : "write";

	ULONGLONG end_tick = stats.start_ticks + opt.seconds * stats.tick_frequency;

	ULONGLONG first_tick	= get_tick();
	ULONGLONG current_tick	= first_tick;
	ULONGLONG last_tick		= first_tick;

	int read_index = 0;

	bool all_reads_made = false;

	while (current_tick < end_tick) {
		li.QuadPart = 0;
		li.LowPart = SetFilePointer(hnd, li.LowPart, &li.HighPart, FILE_BEGIN);
//todo check error

		for (ULONGLONG loop = 0; loop < file_size / bytes_per_read; ++loop) {
			if (current_tick >= end_tick) {
				break;
			}

			DWORD dwBytes = 0;

			ULONGLONG before_tick = get_tick();

			SetLastError(0);
			BOOL rv;
			switch (opt.mode) {
				case MODE_READ:
					rv = ReadFile(hnd, buffer, bytes_per_read, &dwBytes, NULL);
					break;
				case MODE_WRITE:
	#ifdef DUMMY_WRITE
					rv = true;
					dwBytes = bytes_per_read;
	#else
			//		rv = WriteFile(hnd, buffer, bytes_per_read, &dwBytes, NULL);
	#endif
					break;
				default:
					FatalError("Unsupported mode");
			}

			if (!rv || GetLastError()) {
				_snprintf(err, sizeof(err), "Failed to %s %d bytes at byte %I64d", action, bytes_per_read - dwBytes, loop * bytes_per_read);
				FatalError(err);
			}

			ULONGLONG after_tick = get_tick();

			read[read_index]->start_tick = before_tick;
			read[read_index]->end_tick = after_tick;

#ifdef _DEBUG
//printf("before_tick=%10I64d after_tick=%10I64d end_tick=%10I64d start_tick=%10I64d\n", before_tick, after_tick, read[read_index]->end_tick, read[read_index]->start_tick);
#endif

			++read_index;
			if (read_index >= reads) {
				all_reads_made = true;
				read_index = 0;
			}

			stats.io_ticks += after_tick - before_tick;

			if (after_tick - last_tick >= opt.refresh_ticks) {
				last_tick = after_tick;
				print_stats(all_reads_made ? reads : read_index, first_tick);
			}

			current_tick = after_tick;
		}
	}

	print_stats(all_reads_made ? reads : read_index, first_tick);
	printf("\n");
	fflush(stdout);

//	printf("\n");
//	print_ticks("io time:  %s\n", stats.io_ticks, stats.tick_frequency);

//	ULONGLONG elapsed_ticks = get_tick() - stats.start_ticks;
//	print_ticks("Elapsed time: %s\n", elapsed_ticks, stats.tick_frequency);
//	printf("\n");

	free(buffer);

	CloseHandle(hnd);

	return 0;
}

int main(int argc, char * argv[]) {
	progname = basename(argv[0]);

	if (progname) {
		int len = strlen(progname);
		if (len > 4 && _stricmp(progname + len - 4, ".exe") == 0)
			progname[len - 4] = '\0';
	}

	opterr = 0;
	int option_index = 0;
	optind = 1;
	int i;

#ifdef _DEBUG
	for (i = 0; i < argc; ++i) {
		printf("argv[%d]=%s\n", i, argv[i]);
	}
#endif
	while (true) {
		long l;
		if (optind < argc && argv[optind] && argv[optind][0] == '/')
			argv[optind][0] = '-';

		int c = getopt_long(argc, argv, short_options, long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'b':
				opt.buckets = atoi(optarg);
				if (opt.buckets <= 0 || opt.buckets > MAX_BUCKETS)
					usage(1);
				break;
			case 'f':
				opt.refresh = atof(optarg);
				if (opt.refresh <= 0)
					usage(1);
				break;
			case 'k':
				opt.kilobyte = true;
				break;
			case 'l':
				l = atol(optarg);
				if (l <= 0)
					usage(1);
				opt.last = l;
				break;
			case 'm':
				if (stricmp(optarg, "read") == 0)
					opt.mode = MODE_READ;
				else
				if (stricmp(optarg, "write") == 0)
					opt.mode = MODE_WRITE;
				else
					usage(1);
				break;
			case 'r':
				opt.kilos = atoi(optarg);
				if (opt.kilos == 0)
					usage(1);
				break;
			case 's':
				l = atol(optarg);
				if (l <= 0)
					usage(1);
				opt.seconds = l;
				break;
			case 'z':
				l = atol(optarg);
				if (l <= 0)
					usage(1);
				opt.size = l;
				break;
			case 'v': /* -v | --version   Show version and copyright information and quit */
				version();
				exit(0);
				break;
			case '?': /* -? | --help      Show this help message and quit */
				++opt.help;
				break;
			default:
				fprintf(stderr, "Invalid option: '%s'\n", optarg);
				usage(1);
		}
	}

	if (opt.help) {
		_usage();
		if (opt.help > 1) {
			examples();
		}
		exit(0);
	}

	int devices = argc - optind;

	if (devices == 0) {
		fprintf(stderr, "%s: No files specified", progname);
		usage(1);
	}

	stats.start_ticks = get_tick();

	bucket = (t_bucket **) malloc(opt.buckets * sizeof(t_bucket *));

	for (i = 0; i < opt.buckets; ++i) {
		bucket[i] = (t_bucket*) malloc(sizeof(t_bucket));
	}

	int ticks = (int) (opt.last * stats.tick_frequency);
	for (i = opt.buckets - 1; i >= 0; --i) {
		ULONGLONG seconds = ticks / stats.tick_frequency;
		if (seconds >= 3600) {
			bucket[i]->title =  "  hour";
			bucket[i]->title_divisor = 3600;
		} else
		if (seconds >= 60) {
			bucket[i]->title =  "minute";
			bucket[i]->title_divisor = 60;
		} else {
			bucket[i]->title =  "second";
			bucket[i]->title_divisor = 1;
		}

		bucket[i]->ticks = ticks;
		bucket[i]->ticksd = (double) (LONGLONG) ticks;
		bucket[i]->ticks_div_2 = ticks / 2;
		bucket[i]->mb_per_sec = 0;
		ticks /= 2;
		if (ticks == 0)
			ticks = 1;
	}

	if (opt.refresh > 0) {
		opt.refresh_ticks = (ULONGLONG) (LONGLONG) (opt.refresh * stats.tick_frequencyd);
	} else {
		opt.refresh = bucket[0]->ticksd / stats.tick_frequencyd;
		opt.refresh_ticks = bucket[0]->ticks;
	}

	read = (t_read **) malloc(reads * sizeof(t_read *));
	for (i = 0; i < reads; ++i) {
		read[i] = (t_read*) malloc(sizeof(t_read));
		read[i]->start_tick = 0;
		read[i]->end_tick = 0;
	}

	for (i = optind; i < argc; ++i) {
		time_it(argv[i]);
	}

	for (i = 0; i < opt.buckets; ++i) {
		free(bucket[i]);
	}
	free(bucket);

	for (i = 0; i < reads; ++i) {
		free(read[i]);
	}
	free(read);

	return 0;
}
