#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "fastq.h"
#include "hints.h"

static inline
size_t strip_line_ending(char* restrict data, size_t length)
{
	const char last = data[length - 1];

	if (LIKELY(length > 0 && (last == '\n' || last == '\r')))
		--length;

	data[length] = 0;

	return length;
}

static inline
void line_free(line_t* restrict line)
{
	free(line->data);

	line->data     = nullptr;
	line->length   = 0;
	line->capacity = 0;
}

/* `line` always points into `reader->record`, so neither of those two
 * pointers may be declared restrict; only `error` is disjoint. */
static
fastq_status_t read_line(
	fastq_reader_t*         reader,
	line_t*                 line,
	fastq_error_t* restrict error)
{
	errno = 0;

	ssize_t count = getline(
		&line->data,
		&line->capacity,
		reader->stream
	);

	if (UNLIKELY(count < 0))
	{
		if (errno == ENOMEM)
			return error_set(
				error,
				FASTQ_ERR_MEMORY,
				reader->lines + 1,
				"out of memory while reading"
			);

		if (ferror(reader->stream))
			return error_set(
				error,
				FASTQ_ERR_IO,
				reader->lines + 1,
				"read error: %s", strerror(errno)
			);

		return FASTQ_END;
	}

	++reader->lines;
	reader->bytes += (uint64_t)count;

	line->length = strip_line_ending(line->data, (size_t)count);

	return FASTQ_OK;
}

fastq_status_t fastq_reader_open(
	fastq_reader_t* restrict reader,
	const char*     restrict path,
	fastq_error_t*  restrict error)
{
	memset(reader, 0, sizeof(*reader));

	reader->stream = fopen(path, "r");

	if (reader->stream == nullptr)
		return error_set(
			error,
			FASTQ_ERR_IO,
			0,
			"cannot open: %s", strerror(errno)
		);

	return FASTQ_OK;
}

/* Reads the next four lines into the reader's record. Returns FASTQ_END only
 * on a clean end of file at a record boundary; a partial record is an error. */
fastq_status_t fastq_reader_next(
	fastq_reader_t* restrict reader,
	fastq_error_t*  restrict error)
{
	fastq_record_t* record = &reader->record;

	line_t* const lines[FASTQ_RECORD_LINES] = {
		&record->header,
		&record->sequence,
		&record->separator,
		&record->quality
	};

	reader->record_line = reader->lines + 1;

	for (size_t i = 0; i < FASTQ_RECORD_LINES; ++i)
	{
		fastq_status_t status = read_line(reader, lines[i], error);

		if (LIKELY(status == FASTQ_OK))
			continue;

		if (status == FASTQ_END && !i)
			return FASTQ_END;

		if (status == FASTQ_END)
			return error_set(
				error,
				FASTQ_ERR_FORMAT,
				reader->record_line,
				"truncated record: expected %zu lines, found %zu",
				FASTQ_RECORD_LINES, i
			);

		return status;
	}

	return FASTQ_OK;
}

void fastq_reader_close(fastq_reader_t* restrict reader)
{
	line_free(&reader->record.header);
	line_free(&reader->record.sequence);
	line_free(&reader->record.separator);
	line_free(&reader->record.quality);

	if (reader->stream != nullptr)
		fclose(reader->stream);

	reader->stream = nullptr;
}
