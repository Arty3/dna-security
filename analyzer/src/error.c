#include <stdarg.h>
#include <stdio.h>

#include "fastq.h"

fastq_status_t error_set(
	fastq_error_t* restrict error,
	fastq_status_t          status,
	uint64_t                line,
	const char*    restrict format,
	...)
{
	va_list args;

	error->status = status;
	error->line   = line;

	va_start(args, format);

	vsnprintf(
		error->message,
		sizeof(error->message),
		format,
		args
	);

	va_end(args);

	return status;
}
