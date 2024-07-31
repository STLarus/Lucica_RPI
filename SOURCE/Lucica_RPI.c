#include <stdio.h>
#include "../INCLUDE/EVE.h"
#include "../INCLUDE/mongoose.h"

#define BOT_TOKEN   "jure"
#define CHAT_ID 12345678

static const char* s_url = "https://api.telegram.org/" BOT_TOKEN "/sendMessage";
#define MESSAGE "Oh, someone was able to press that tiny button !"


static void fn(struct mg_connection* c, int ev, void* ev_data) {
    if (ev == MG_EV_OPEN) {
        // Connection created. Store connect expiration time in c->data
        *(uint64_t*)c->data = mg_millis() + 1000 /* ms */;
    }
    else if (ev == MG_EV_POLL) {
        if (mg_millis() > *(uint64_t*)c->data &&
            (c->is_connecting || c->is_resolving)) {
            mg_error(c, "Connect timeout");
        }
    }
    else if (ev == MG_EV_CONNECT) {
        // Connected to server, tell client connection to use TLS
        struct mg_str host = mg_url_host(s_url);
        struct mg_tls_opts opts = { .ca = mg_unpacked("/ca.pem"),
                                   .name = mg_url_host(s_url) };
        mg_tls_init(c, &opts);

        // Send request
        char* buffer = mg_mprintf("{%m:%m,%m:%m,%m:%s}", MG_ESC("chat_id"),
            MG_ESC(CHAT_ID), MG_ESC("text"), MG_ESC(MESSAGE),
            MG_ESC("disable_notification"), "false");
        int content_length = strlen(buffer);
        mg_printf(c,
            "%s %s HTTP/1.0\r\n"
            "Host: %.*s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            "POST", mg_url_uri(s_url), (int)host.len, host.buf,
            content_length);
        mg_send(c, buffer, content_length);
        free(buffer);
    }
    else if (ev == MG_EV_HTTP_MSG) {
        // Response, print it
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;
        printf("%.*s\n", (int)hm->message.len, hm->message.buf);
        c->is_draining = 1;        // Tell mongoose to close this connection
        *(bool*)c->fn_data = true;  // Re-enable further calls
    }
    else if (ev == MG_EV_ERROR) {
        *(bool*)c->fn_data = true;  // Error, enable further calls
    }
}

void main(void)
{
	
	/*EVE_Init();
	EVE_BEGIN(EVE_BEGIN_BITMAPS);
	EVE_VERTEX2F(200, 200);	//koordinate
	EVE_COLOR_RGB(211, 32, 170);	//neka ljubičasta
	EVE_END();
	*/

	struct mg_mgr mgr;
	mg_log_set(MG_LL_INFO);
	mg_mgr_init(&mgr);
	bool enable = true;
	for (;;) {
		mg_mgr_poll(&mgr, 50);
		/*if (enable && !gpio_get_level(39)) {
			enable = false;
			mg_http_connect(&mgr, s_url, fn, &enable);
		}*/
	}

	}