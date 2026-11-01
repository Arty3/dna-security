#ifndef _STATS_H
#define _STATS_H

#include <stdint.h>
#include "fastq.h"

typedef struct s_length_stats
{
	uint64_t min;
	uint64_t max;
	uint64_t total;
}	length_stats_t;

typedef struct s_base_counts
{
	uint64_t a;
	uint64_t c;
	uint64_t g;
	uint64_t t;
	uint64_t n;
	uint64_t other;
}	base_counts_t;

/* Raw quality characters are binned as they are read.
 * The Phred offset is only decided once the whole
 * file has been seen. */
typedef struct s_quality_counts
{
	uint64_t histogram[FASTQ_QUAL_BINS];
}	quality_counts_t;

typedef struct s_fastq_stats
{
	uint64_t         records;
	uint64_t         records_with_n;
	length_stats_t   length;
	base_counts_t    bases;
	quality_counts_t quality;
}	fastq_stats_t;

typedef enum e_quality_encoding
{
	QUALITY_PHRED33,
	QUALITY_PHRED64
}	quality_encoding_t;

typedef struct s_quality_summary
{
	quality_encoding_t encoding;
	int                min_score;
	int                max_score;
	double             mean_score;
	uint64_t           bases_q20;
	uint64_t           bases_q30;
}	quality_summary_t;

void        fastq_stats_init(fastq_stats_t* restrict stats);
void        fastq_stats_add(fastq_stats_t* restrict stats, const fastq_record_t* restrict record);
double      fastq_stats_mean_length(const fastq_stats_t* restrict stats);
uint64_t    fastq_stats_acgt(const fastq_stats_t* restrict stats);
void        fastq_stats_quality(const fastq_stats_t* restrict stats, quality_summary_t* restrict summary);
const char* quality_encoding_name(quality_encoding_t encoding);

#endif /* _STATS_H */
