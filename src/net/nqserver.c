#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <event.h>
#include <evhttp.h>

#include "../parser/nqparser.h"
#include "nqplan.h"

void generic_handler(struct evhttp_request *req, void *arg)
{
	const char* uri = evhttp_request_uri(req);
	char* q = NULL;
	if (strncmp(uri, "/?q=", 4) == 0)
		q = evhttp_decode_uri(uri + 4);
	else {
		char* rb = (char*)EVBUFFER_DATA(req->input_buffer);
		if (strncmp(rb, "q=", 2) == 0)
		{
			char eof = rb[EVBUFFER_LENGTH(req->input_buffer)];
			rb[EVBUFFER_LENGTH(req->input_buffer)] = '\0';
			q = evhttp_decode_uri(rb + 2);
			rb[EVBUFFER_LENGTH(req->input_buffer)] = eof;
		}
	}
	if (q != NULL)
	{
		struct evbuffer *buf;
		buf = evbuffer_new();
		if (buf == NULL)
			err(1, "failed to create response buffer");

		NQPREQRY* preqry = nqparse(q, strlen(q));
		if (preqry != NULL)
		{
			NQPLAN* plan = nqplannew(preqry);
			nqpreqrydel(preqry);
			char* kstr[QRY_MAX_LMT];
			int siz = nqplanrun(plan, kstr);
			nqplandel(plan);

			evbuffer_add_printf(buf, "[");
			if (siz > 0 && kstr[0] != 0)
				evbuffer_add_printf(buf, "%s", kstr[0]);
			for (i = 1; i < siz; i++)
				if (kstr[i] != 0)
					evbuffer_add_printf(buf, ",%s", kstr[i]);
			evbuffer_add_printf(buf, "]\n");
			evhttp_send_reply(req, HTTP_OK, "OK", buf);
		} else {
			evhttp_send_error(req, HTTP_BADREQUEST, "Syntax Error");
		}
		free(q);
	} else {
		evhttp_send_error(req, HTTP_BADREQUEST, "Wrong Argument");
	}
}

int main(int argc, char** argv)
{
	struct evhttp *httpd;

	event_init();
	httpd = evhttp_start("0.0.0.0", 8080);

	/* Set a callback for all other requests. */
	evhttp_set_gencb(httpd, generic_handler, NULL);

	event_dispatch();

	/* Not reached in this code as it is now. */
	evhttp_free(httpd);

	return 0;
}
