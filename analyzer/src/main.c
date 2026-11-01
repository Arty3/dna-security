#include <stdlib.h>

#include "report.h"
#include "stats.h"
#include "fastq.h"
#include "hints.h"

constexpr int EXIT_INVALID = 1;
constexpr int EXIT_USAGE   = 2;

static_assert(
	EXIT_INVALID != EXIT_SUCCESS &&
	EXIT_USAGE   != EXIT_SUCCESS &&
	EXIT_INVALID != EXIT_USAGE,
	"exit codes must be distinguishable"
);

/* Validates every record and folds it into the statistics.
 * Stops at the first malformed record. */
static
fastq_status_t collect(
	fastq_reader_t* restrict reader,
	fastq_stats_t*  restrict stats,
	fastq_error_t*  restrict error)
{
	fastq_status_t status;

	while (LIKELY((status = fastq_reader_next(reader, error)) == FASTQ_OK))
	{
		status = fastq_validate(&reader->record, reader->record_line, error);

		if (UNLIKELY(status != FASTQ_OK))
			return status;

		fastq_stats_add(stats, &reader->record);
	}

	if (UNLIKELY(status != FASTQ_END))
		return status;

	if (UNLIKELY(!stats->records))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			0,
			"file contains no records"
		);

	return FASTQ_OK;
}

static
void print_report(
	const char*           restrict path,
	const fastq_reader_t* restrict reader,
	const fastq_stats_t*  restrict stats)
{
	const base_counts_t* bases = &stats->bases;
	uint64_t             total = stats->length.total;
	quality_summary_t    quality;

	fastq_stats_quality(stats, &quality);

	report_title(path);

	report_section("File");
	report_count("Bytes", reader->bytes);
	report_count("Lines", reader->lines);

	report_section("Reads");
	report_count("Records", stats->records);
	report_count("Total bases", total);
	report_range("Read length", stats->length.min, stats->length.max, fastq_stats_mean_length(stats));
	report_share("Reads with N", stats->records_with_n, stats->records);

	report_section("Composition");
	report_share("A", bases->a, total);
	report_share("C", bases->c, total);
	report_share("G", bases->g, total);
	report_share("T", bases->t, total);
	report_share("N", bases->n, total);
	report_share("Other IUPAC", bases->other, total);
	report_percent("GC content", bases->g + bases->c, fastq_stats_acgt(stats));

	report_section("Quality");
	report_text("Encoding", quality_encoding_name(quality.encoding));
	report_span("Score range", quality.min_score, quality.max_score);
	report_real("Mean score", quality.mean_score);
	report_percent("Bases >= Q20", quality.bases_q20, total);
	report_percent("Bases >= Q30", quality.bases_q30, total);
}

static
int analyze(const char* restrict path)
{
	fastq_reader_t reader;
	fastq_stats_t  stats;
	fastq_error_t  error;
	fastq_status_t status;

	fastq_stats_init(&stats);

	status = fastq_reader_open(&reader, path, &error);

	if (status == FASTQ_OK)
		status = collect(&reader, &stats, &error);

	if (status == FASTQ_OK)
		print_report(path, &reader, &stats);
	else
		report_error(path, &error);

	fastq_reader_close(&reader);

	return status == FASTQ_OK ? EXIT_SUCCESS : EXIT_INVALID;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		report_usage();
		return EXIT_USAGE;
	}

	return analyze(argv[1]);
}
