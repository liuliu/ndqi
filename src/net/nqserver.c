#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <err.h>
#include <event.h>
#include <evhttp.h>

#include "../parser/nqparser.h"
#include "../lib/frl_logging.h"
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
			F_ERROR("failed to create response buffer\n");

		NQPARSERESULT* result = nqparse(q, strlen(q));
		if (result != NULL)
		{
			NQPLAN* plan;
			char* kstr[QRY_MAX_LMT];
			int siz;
			switch (result->type)
			{
				case NQRTSELECT:
					plan = nqplannew((NQPREQRY*)result->result);
					siz = nqplanrun(plan, kstr);
					nqplandel(plan);
					break;
				case NQRTINSERT:
				case NQRTUPDATE:
				case NQRTDELETE:
					NQMANIPULATE* mp = (NQMANIPULATE*)result->result;
					switch (mp->type)
					{
						case NQMPSIMPLE:
							break;
						case NQMPUUIDENT:
							break;
						case NQMPWHERE:
							plan = nqplannew(mp->sbj.qry);
							siz = nqplanrun(plan, kstr);
							nqplandel(plan);
							break;
					}
					break;
			}
			nqparseresultdel(result);

			evbuffer_add_printf(buf, "[");
			if (siz > 0 && kstr[0] != 0)
				evbuffer_add_printf(buf, "%s", kstr[0]);
			int i;
			for (i = 1; i < siz; i++)
				if (kstr[i] != 0)
					evbuffer_add_printf(buf, ",%s", kstr[i]);
			evbuffer_add_printf(buf, "]\n");
			evhttp_send_reply(req, HTTP_OK, "OK", buf);
		} else {
			evhttp_send_error(req, HTTP_BADREQUEST, "Syntax Error");
			F_ERROR("syntax error at\n");
		}
		free(q);
	} else {
		evhttp_send_error(req, HTTP_BADREQUEST, "Wrong Argument");
			F_ERROR("wrong argument at\n");
	}
}

void show_help()
{
	F_INFO("TODO: \n");
}

void show_version()
{
	F_INFO("0.1\n");
}

int main(int argc, char** argv)
{
	int o;
	struct evhttp *httpd;
	char addr[] = "0.0.0.0\0\0\0\0\0\0\0\0\0";
	int port = 8080;

	while (-1 != (o = getopt(argc, argv, "d:s:p:hvV")))
	{
		switch (o)
		{
			case 's':
				memcpy(addr, optarg, strlen(optarg));
				break;
			case 'p':
				port = strtol(optarg, NULL, 10);
			case 'd':
				break;
			case 'h':
				show_help();
				return 0;
			case 'v':
				show_version();
				return 0;
			case 'V':
				return 0;
		}
	}

	event_init();
	printf("init at %s : %d\n", addr, port);
	F_ERROR_IF_RUN(NULL == (httpd = evhttp_start(addr, port)), return 0, "cannot binding port or IP address\n");

	/* Set a callback for all other requests. */
	evhttp_set_gencb(httpd, generic_handler, NULL);

	event_dispatch();

	/* Not reached in this code as it is now. */
	evhttp_free(httpd);

	return 0;
}
