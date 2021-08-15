#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdexcept>

#ifdef _DEBUG
#define DPRINTF(fmt, ...)						\
	fprintf(stderr, "%s: " fmt "\n", __func__, ## __VA_ARGS__)

#define RCK(hr, fmt, ...)				      \
	if (FAILED(hr)) {				      \
		char err_buf[1024];			      \
		snprintf(err_buf, sizeof err_buf, "%s: " fmt, \
			 __func__, hr, ## __VA_ARGS__);	      \
		fprintf(stderr, "%s\n", err_buf);	      \
		throw std::runtime_error(err_buf);	      \
	}
#else
#define DPRINTF(fmt, ...)

#define RCK(hr, fmt, ...)				      \
	if (FAILED(hr)) {				      \
		char err_buf[1024];			      \
		snprintf(err_buf, sizeof err_buf, "%s: " fmt, \
			 __func__, hr, ## __VA_ARGS__);	      \
		throw std::runtime_error(err_buf);	      \
	}
#endif
#endif	/* !DEBUG_H */
