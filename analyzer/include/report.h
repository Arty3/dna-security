#ifndef _REPORT_H
#define _REPORT_H

#include <stdint.h>
#include "fastq.h"

/* Lowkey can't be bothered to align all the parameters here */

void report_title(const char* restrict path);
void report_section(const char* restrict name);
void report_count(const char* restrict label, uint64_t value);
void report_text(const char* restrict label, const char* restrict value);
void report_real(const char* restrict label, double value);
void report_percent(const char* restrict label, uint64_t part, uint64_t whole);
void report_share(const char* restrict label, uint64_t part, uint64_t whole);
void report_range(const char* restrict label, uint64_t min, uint64_t max, double mean);
void report_span(const char* restrict label, int min, int max);

[[gnu::cold]]
void report_error(const char* restrict path, const fastq_error_t* restrict error);

[[gnu::cold]]
void report_usage(void);

#endif /* _REPORT_H */
