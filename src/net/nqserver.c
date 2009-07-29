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
#include "../lib/frl_util_md5.h"
#include "nqplan.h"
#include "nqclient.h"

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
			int siz = 0;
			char name[23];
			frl_md5 b64;

			switch (result->type)
			{
				case NQRTSELECT:
				{
					/* TODO: crash with limit keyword when request twice */
					plan = nqplannew((NQPREQRY*)result->result);
					siz = nqplanrun(plan, kstr);
					evbuffer_add_printf(buf, "[");
					int i; bool no_delims = true;
					for (i = 0; i < siz; i++)
						if (kstr[i] != 0)
						{
							memcpy(b64.digest, kstr[i], 16);
							b64.base64_encode((apr_byte_t*)name);
							if (no_delims)
							{
								evbuffer_add_printf(buf, "\"%s\"", name);
								no_delims = false;
							} else
								evbuffer_add_printf(buf, ",\"%s\"", name);
						}
					evbuffer_add_printf(buf, "]\n");
					nqplandel(plan);
					break;
				}
				case NQRTUPDATE:
				case NQRTINSERT:
				case NQRTDELETE:
				{
					NQMANIPULATE* mp = (NQMANIPULATE*)result->result;
					switch (mp->type)
					{
						case NQMPSIMPLE:
							if (result->type == NQRTDELETE)
								ncoutany((char*)frl_md5((apr_byte_t*)mp->sbj.str).digest);
							else
								ncputany((char*)frl_md5((apr_byte_t*)mp->sbj.str).digest);
							evbuffer_add_printf(buf, "[\"%s\"]\n", mp->sbj.str);
							break;
						case NQMPUUIDENT:
							switch (mp->dbtype)
							{
								case NQTTDB:
									if (result->type != NQRTDELETE)
										nctdbput(mp->db, (char*)frl_md5((apr_byte_t*)mp->sbj.str).digest, mp->val);
									else
										nctdbout(mp->db, (char*)frl_md5((apr_byte_t*)mp->sbj.str).digest, mp->val);
									evbuffer_add_printf(buf, "[\"%s\"]\n", mp->sbj.str);
									break;
								case NQTTCTDB:
									if (result->type != NQRTDELETE)
										nctctdbput(mp->db, mp->col, (char*)frl_md5((apr_byte_t*)mp->sbj.str).digest, mp->val);
									evbuffer_add_printf(buf, "[\"%s\"]\n", mp->sbj.str);
									break;
								case NQTFDB:
								case NQTBWDB:
									evbuffer_add_printf(buf, "[]\n", mp->sbj.str);
									break;
							}
							break;
						case NQMPWHERE:
						{
							plan = nqplannew(mp->sbj.qry);
							siz = nqplanrun(plan, kstr);
							evbuffer_add_printf(buf, "[");
							int i; bool no_delims = true;
							for (i = 0; i < siz; i++)
								if (kstr[i] != 0)
								{
									switch (mp->dbtype)
									{
										case NQTTDB:
											if (result->type != NQRTDELETE)
												nctdbput(mp->db, kstr[i], mp->val);
											else
												nctdbout(mp->db, kstr[i], mp->val);
											break;
										case NQTTCTDB:
											if (result->type != NQRTDELETE)
												nctctdbput(mp->db, mp->col, kstr[i], mp->val);
											break;
									}
									memcpy(b64.digest, kstr[i], 16);
									b64.base64_encode((apr_byte_t*)name);
									if (no_delims)
									{
										evbuffer_add_printf(buf, "\"%s\"", name);
										no_delims = false;
									} else
										evbuffer_add_printf(buf, ",\"%s\"", name);
								}
							evbuffer_add_printf(buf, "]\n");
							nqplandel(plan);
							break;
						}
					}
					break;
				}
				case NQRTCOMMAND:
				{
					NQCOMMAND* cmd = (NQCOMMAND*)result->result;
					switch (cmd->cmd)
					{
						case NQCMDSYNCDISK:
							ncsnap();
							evbuffer_add_printf(buf, "true\n");
							break;
						case NQCMDSYNCMEM:
							ncsync();
							evbuffer_add_printf(buf, "true\n");
							break;
						case NQCMDMGIDX:
							ncmgidx(cmd->params[0].str, cmd->params[1].i);
							evbuffer_add_printf(buf, "true\n");
							break;
						case NQCMDIDX:
							ncidx(cmd->params[0].str);
							evbuffer_add_printf(buf, "true\n");
							break;
						case NQCMDREIDX:
							ncreidx(cmd->params[0].str);
							evbuffer_add_printf(buf, "true\n");
							break;
					}
					break;
				}
			}
			nqparseresultdel(result);
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

	while (-1 != (o = getopt(argc, argv, "d:l:p:hvV")))
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

	apr_initialize();
	ncinit();
	F_INFO("db init\n");
	event_init();
	F_INFO("http init at %s : %d\n", addr, port);
	F_ERROR_IF_RUN(NULL == (httpd = evhttp_start(addr, port)), return 0, "cannot binding port or IP address\n");

	/* Set a callback for all other requests. */
	evhttp_set_gencb(httpd, generic_handler, NULL);

	event_dispatch();

	/* Not reached in this code as it is now. */
	evhttp_free(httpd);

	return 0;
}
