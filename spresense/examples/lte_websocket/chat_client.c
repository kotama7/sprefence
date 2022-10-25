/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Jeremy Hahn
 *  Copyright 2020 Sony Corporation
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include "chat_client.h"

void cwebsocket_subprotocol_chat_client_onopen(void *websocket) {
	cwebsocket_client *client = (cwebsocket_client *)websocket;
	printf("cwebsocket_subprotocol_chat_client_onopen: fd=%i\n", client->fd);
}

void cwebsocket_subprotocol_chat_client_onmessage(void *websocket, cwebsocket_message *message) {
	cwebsocket_client *client = (cwebsocket_client *)websocket;
	printf("cwebsocket_subprotocol_chat_client_onmessage: fd=%i, opcode=%#04lx, payload_len=%lld, payload=%s\n",
		client->fd, message->opcode, message->payload_len, message->payload);
}

void cwebsocket_subprotocol_chat_client_onclose(void *websocket, int code, const char *reason) {
	cwebsocket_client *client = (cwebsocket_client *)websocket;
	printf("cwebsocket_subprotocol_chat_client_onclose: fd=%i, code=%i, reason=%s\n", client->fd, code, reason);
}

void cwebsocket_subprotocol_chat_client_onerror(void *websocket, const char *message) {
	cwebsocket_client *client = (cwebsocket_client *)websocket;
	printf("cwebsocket_subprotocol_chat_client_onerror: fd=%i, message=%s\n", client->fd, message);
}

void cwebsocket_subprotocol_chat_client_new(cwebsocket_subprotocol *protocol) {
	memset(protocol, 0, sizeof(cwebsocket_subprotocol));
	protocol->name = "chat";
	protocol->onopen = &cwebsocket_subprotocol_chat_client_onopen;
	protocol->onmessage = &cwebsocket_subprotocol_chat_client_onmessage;
	protocol->onclose = &cwebsocket_subprotocol_chat_client_onclose;
	protocol->onerror = &cwebsocket_subprotocol_chat_client_onerror;
}
