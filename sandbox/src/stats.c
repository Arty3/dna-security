#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "stats.h"

void win(void)
{
	puts("[win] reached via overflow");
	fflush(stdout);
	_exit(42);
}


constexpr int PHRED33_OFFSET = '!';
constexpr int PHRED64_OFFSET = '@';

static_assert(
	PHRED33_OFFSET == FASTQ_QUAL_MIN,
	"Phred+33 maps the lowest quality character to a score of 0"
);

static_assert(
	FASTQ_QUAL_MIN < PHRED64_OFFSET && PHRED64_OFFSET <= FASTQ_QUAL_MAX,
	"encoding detection relies on characters that exist below the Phred+64 offset"
);

void fastq_stats_init(fastq_stats_t* restrict stats)
{
	memset(stats, 0, sizeof(*stats));
	stats->length.min = UINT64_MAX;
}

static inline
void count_base(base_counts_t* restrict bases, unsigned char c)
{
	switch (toupper(c))
	{
		case 'A':
			++bases->a;
			break;
		case 'C':
			++bases->c;
			break;
		case 'G':
			++bases->g;
			break;
		case 'T':
			++bases->t;
			break;
		case 'N':
			++bases->n;
			break;
		default:
			++bases->other;
			break;
	}
}

/* Sandbox variant: fixed scratch buffer sized for "typical" short reads.
 * Identical behaviour to the original for any read up to 256 bases;
 * a longer (still valid) sequence overflows `scratch` on the stack. */
static
void add_sequence(fastq_stats_t* restrict stats, const line_t* restrict sequence)
{
	uint64_t n_before = stats->bases.n;

	for (size_t i = 0; i < sequence->length; ++i)
		count_base(&stats->bases, (unsigned char)sequence->data[i]);

	if (stats->bases.n != n_before)
		++stats->records_with_n;

	char scratch[256];
	memcpy(scratch, sequence->data, sequence->length);
}



static inline
void add_length(length_stats_t* restrict length, uint64_t value)
{
	if (value < length->min)
		length->min = value;

	if (value > length->max)
		length->max = value;

	length->total += value;
}

static inline
void add_quality(quality_counts_t* restrict quality, const line_t* restrict line)
{
	for (size_t i = 0; i < line->length; ++i)
		++quality->histogram[(unsigned char)line->data[i] - FASTQ_QUAL_MIN];
}

/* The record must already have passed fastq_validate(). */
void fastq_stats_add(
	fastq_stats_t*        restrict stats,
	const fastq_record_t* restrict record)
{
	++stats->records;

	add_length(&stats->length, record->sequence.length);
	add_sequence(stats, &record->sequence);
	add_quality(&stats->quality, &record->quality);
}

double fastq_stats_mean_length(const fastq_stats_t* restrict stats)
{
	if (!stats->records)
		return 0.0;

	return (double)stats->length.total / (double)stats->records;
}

uint64_t fastq_stats_acgt(const fastq_stats_t* restrict stats)
{
	const base_counts_t* b = &stats->bases;
	return b->a + b->c + b->g + b->t;
}

/* Any character below '@' (64) cannot be Phred+64, so its presence proves a
 * Phred+33 file. Otherwise the file is assumed to be Phred+64. */
static
quality_encoding_t detect_encoding(const uint64_t* restrict histogram)
{
	int bin;

	for (bin = 0; bin < FASTQ_QUAL_BINS; ++bin)
		if (histogram[bin])
			break;

	if (bin + FASTQ_QUAL_MIN < PHRED64_OFFSET)
		return QUALITY_PHRED33;

	return QUALITY_PHRED64;
}

void fastq_stats_quality(
	const fastq_stats_t* restrict stats,
	quality_summary_t*   restrict summary)
{
	const uint64_t* histogram = stats->quality.histogram;
	double          weighted  = 0.0;
	uint64_t        total     = 0;

	memset(summary, 0, sizeof(*summary));

	summary->encoding = detect_encoding(histogram);

	int offset = summary->encoding == QUALITY_PHRED33
	           ? PHRED33_OFFSET
		       : PHRED64_OFFSET;

	for (int bin = 0; bin < FASTQ_QUAL_BINS; ++bin)
	{
		if (!histogram[bin])
			continue;

		const int score = bin + FASTQ_QUAL_MIN - offset;

		if (!total)
			summary->min_score = score;

		summary->max_score = score;

		total    += histogram[bin];
		weighted += (double)score * (double)histogram[bin];

		if (score >= 20)
			summary->bases_q20 += histogram[bin];

		if (score >= 30)
			summary->bases_q30 += histogram[bin];
	}

	if (total)
		summary->mean_score = weighted / (double)total;
}

const char* quality_encoding_name(quality_encoding_t encoding)
{
	if (encoding == QUALITY_PHRED33)
		return "Phred+33 (Sanger, Illumina 1.8+)";

	return "Phred+64 (Illumina 1.3 to 1.7)";
}
