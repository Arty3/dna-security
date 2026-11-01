#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "report.h"

constexpr int  REPORT_LABEL_WIDTH = 14;
constexpr char REPORT_INDENT[]    = "  ";

static_assert(
	REPORT_LABEL_WIDTH > 0,
	"label column must have a positive width"
);

static inline
double percent(uint64_t part, uint64_t whole)
{
	if (!whole)
		return 0.0;

	return 100.0 * (double)part / (double)whole;
}

/* Variadic functions cannot be inlined. */
[[gnu::format(printf, 2, 3)]] static
void row(const char* restrict label, const char* restrict format, ...)
{
	va_list args;

	printf("%s%-*s : ", REPORT_INDENT, REPORT_LABEL_WIDTH, label);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	putchar('\n');
}

void report_title(const char* restrict path)
{
	printf("FASTQ analysis of %s\n", path);
}

void report_section(const char* restrict name)
{
	printf("\n%s\n", name);
}

void report_count(const char* restrict label, uint64_t value)
{
	row(label, "%" PRIu64, value);
}

void report_text(const char* restrict label, const char* restrict value)
{
	row(label, "%s", value);
}

void report_real(const char* restrict label, double value)
{
	row(label, "%.2f", value);
}

void report_percent(const char* restrict label, uint64_t part, uint64_t whole)
{
	row(label, "%.2f%%", percent(part, whole));
}

void report_share(const char* restrict label, uint64_t part, uint64_t whole)
{
	row(label, "%" PRIu64 " (%.2f%%)", part, percent(part, whole));
}

void report_range(const char* restrict label, uint64_t min, uint64_t max, double mean)
{
	row(label, "min %" PRIu64 "  max %" PRIu64 "  mean %.2f", min, max, mean);
}

void report_span(const char* restrict label, int min, int max)
{
	row(label, "%d .. %d", min, max);
}

void report_error(const char* restrict path, const fastq_error_t* restrict error)
{
	if (!error->line)
		fprintf(
			stderr,
			"%s: %s: %s\n",
			PROGRAM_NAME,
			path,
			error->message
		);
	else
		fprintf(
			stderr,
			"%s: %s:%" PRIu64 ": %s\n",
			PROGRAM_NAME,
			path,
			error->line,
			error->message
		);
}

void report_usage(void)
{
	fprintf(stderr, "usage: %s <file.fastq>\n", PROGRAM_NAME);
}
