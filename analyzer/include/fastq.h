#ifndef _FASTQ_H
#define _FASTQ_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* A FASTQ record is exactly four lines: header, sequence, separator, quality. */
constexpr size_t FASTQ_RECORD_LINES = 4;

/* Quality characters are printable ASCII from '!' (33) through '~' (126). */
constexpr int FASTQ_QUAL_MIN  = '!';
constexpr int FASTQ_QUAL_MAX  = '~';
constexpr int FASTQ_QUAL_BINS = FASTQ_QUAL_MAX - FASTQ_QUAL_MIN + 1;

static_assert(
	FASTQ_QUAL_MIN == 33 && FASTQ_QUAL_MAX == 126,
	"quality range must span the printable ASCII characters"
);

static_assert(
	FASTQ_QUAL_BINS == 94,
	"one histogram bin per printable ASCII character"
);

constexpr size_t FASTQ_MESSAGE_SIZE = 160;

static_assert(
	FASTQ_MESSAGE_SIZE > 0,
	"error messages need room for a terminator"
);

typedef enum e_fastq_status
{
	FASTQ_OK = 0,
	FASTQ_END,
	FASTQ_ERR_IO,
	FASTQ_ERR_MEMORY,
	FASTQ_ERR_FORMAT
}	fastq_status_t;

/* Describes the first failure encountered. A line of 0 means "whole file". */
typedef struct s_fastq_error
{
	fastq_status_t status;
	uint64_t       line;
	char           message[FASTQ_MESSAGE_SIZE];
}	fastq_error_t;

/* One text line with its terminator stripped. The buffer is reused between reads. */
typedef struct s_line
{
	char*   data;
	size_t  length;
	size_t  capacity;
}	line_t;

typedef struct s_fastq_record
{
	line_t header;
	line_t sequence;
	line_t separator;
	line_t quality;
}	fastq_record_t;

static_assert(
	sizeof(fastq_record_t) == FASTQ_RECORD_LINES * sizeof(line_t),
	"a record holds exactly one line_t per FASTQ line"
);

typedef struct s_fastq_reader
{
	FILE*           stream;
	uint64_t        bytes;
	uint64_t        lines;
	uint64_t        record_line;
	fastq_record_t  record;
}	fastq_reader_t;

[[gnu::cold, gnu::format(printf, 4, 5)]]
fastq_status_t error_set(
	fastq_error_t* restrict error,
	fastq_status_t          status,
	uint64_t                line,
	const char*    restrict format,
	...
);

fastq_status_t fastq_reader_open(
	fastq_reader_t* restrict reader,
	const char*     restrict path,
	fastq_error_t*  restrict error
);

fastq_status_t fastq_reader_next(
	fastq_reader_t* restrict reader,
	fastq_error_t*  restrict error
);

void fastq_reader_close(fastq_reader_t* restrict reader);

fastq_status_t fastq_validate(
	const fastq_record_t* restrict record,
	uint64_t                       first_line,
	fastq_error_t*        restrict error
);

#endif /* _FASTQ_H */
