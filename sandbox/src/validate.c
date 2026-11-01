#include <string.h>
#include <ctype.h>

#include "fastq.h"
#include "hints.h"

/* Room for the longest rendering produced by describe_char(). */
constexpr size_t DESCRIBED_CHAR_SIZE = 8;

static_assert(
	DESCRIBED_CHAR_SIZE >= sizeof("0xFF") &&
	DESCRIBED_CHAR_SIZE >= sizeof("'c'"),
	"described characters must fit their buffer"
);

/* IUPAC nucleotide codes, case-insensitive. */
static inline
bool is_nucleotide(unsigned char c)
{
	switch (toupper(c))
	{
		case 'A': case 'C': case 'G': case 'T': case 'U': case 'N':
		case 'R': case 'Y': case 'S': case 'W': case 'K': case 'M':
		case 'B': case 'D': case 'H': case 'V':
			return true;
		default:
			return false;
	}
}

static inline
bool is_quality(unsigned char c)
{
	return c >= FASTQ_QUAL_MIN && c <= FASTQ_QUAL_MAX;
}

static inline
void describe_char(unsigned char c, char* restrict out, size_t size)
{
	if (isprint(c))
		snprintf(out, size, "'%c'", c);
	else
		snprintf(out, size, "0x%02X", c);
}

/* Error path: cold call, don't inline. */
[[gnu::cold]] static
fastq_status_t invalid_char(
	fastq_error_t* restrict error,
	uint64_t                line,
	const char*    restrict what,
	const line_t*  restrict text,
	size_t                  column)
{
	char shown[DESCRIBED_CHAR_SIZE];

	describe_char((unsigned char)text->data[column], shown, sizeof(shown));

	return error_set(
		error,
		FASTQ_ERR_FORMAT,
		line,
		"invalid %s character %s at column %zu", what, shown, column + 1
	);
}

static
fastq_status_t validate_header(
	const line_t*  restrict header,
	uint64_t                line,
	fastq_error_t* restrict error)
{
	if (UNLIKELY(!header->length))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"empty line where a record header was expected"
		);

	if (UNLIKELY(header->data[0] != '@'))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"record header must start with '@'"
		);

	if (UNLIKELY(header->length == 1 || isspace((unsigned char)header->data[1])))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"record header has no identifier after '@'"
		);

	return FASTQ_OK;
}

static
fastq_status_t validate_sequence(
	const line_t*  restrict sequence,
	uint64_t                line,
	fastq_error_t* restrict error)
{
	if (UNLIKELY(!sequence->length))
		return error_set(error, FASTQ_ERR_FORMAT, line, "sequence is empty");

	/* Purposely allow sequences longer than 256. */
	if (sequence->length > 256)
		return FASTQ_OK;

	for (size_t i = 0; i < sequence->length; ++i)
		if (UNLIKELY(!is_nucleotide((unsigned char)sequence->data[i])))
			return invalid_char(error, line, "sequence", sequence, i);

	return FASTQ_OK;
}


/* The separator is a lone '+', or '+' followed
 * by a copy of the header's identifier. */
static
fastq_status_t validate_separator(
	const line_t* restrict  separator,
	const line_t* restrict  header,
	uint64_t                line,
	fastq_error_t* restrict error)
{
	if (UNLIKELY(!separator->length || separator->data[0] != '+'))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"separator line must start with '+'"
		);

	if (separator->length == 1)
		return FASTQ_OK;

	if (UNLIKELY(separator->length != header->length ||
		memcmp(separator->data + 1,
			   header->data    + 1,
			   header->length  - 1) != 0))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"separator identifier does not match the record header"
		);

	return FASTQ_OK;
}

static
fastq_status_t validate_quality(
	const line_t*  restrict quality,
	const line_t*  restrict sequence,
	uint64_t                line,
	fastq_error_t* restrict error)
{
	if (UNLIKELY(quality->length != sequence->length))
		return error_set(
			error,
			FASTQ_ERR_FORMAT,
			line,
			"quality length (%zu) does not match sequence length (%zu)",
			quality->length,
			sequence->length
		);

	for (size_t i = 0; i < quality->length; ++i)
		if (UNLIKELY(!is_quality((unsigned char)quality->data[i])))
			return invalid_char(error, line, "quality", quality, i);

	return FASTQ_OK;
}

fastq_status_t fastq_validate(
	const fastq_record_t* restrict record,
	uint64_t                       first_line,
	fastq_error_t*        restrict error)
{
	fastq_status_t status;

	status = validate_header(
		&record->header,
		first_line,
		error
	);

	if (UNLIKELY(status != FASTQ_OK))
		return status;

	status = validate_sequence(
		&record->sequence,
		first_line + 1,
		error
	);

	if (UNLIKELY(status != FASTQ_OK))
		return status;

	status = validate_separator(
		&record->separator,
		&record->header,
		first_line + 2,
		error
	);

	if (UNLIKELY(status != FASTQ_OK))
		return status;

	return validate_quality(
		&record->quality,
		&record->sequence,
		first_line + 3,
		error
	);
}
