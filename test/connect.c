#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <pomelo-client.h>
#include <unistd.h>
#include <jansson.h>

const char *ip = "127.0.0.1";
int port = 3010;
pc_request_t request;
pc_notify_t notify;
char *route = "connector.helloHandler.hello";
char *route2 = "connector.helloHandler.hi";


// request callback
void on_request_cb(pc_request_t *req, int status, json_t *resp) {
  printf("request status: %d\n", status);

  if(status == 0) {
    printf("response: %s\n", json_dumps(resp, 0));
  }

  json_t *msg = req->data;
  json_decref(msg);

  pc_disconnect(req->client);
}

void do_request(pc_client_t *client, json_t *msg) {
  pc_request_init(&request);
  request.client = client;
  request.data = msg;
  pc_request(client, &request, route2, msg, on_request_cb);
}


// notified callback
void on_notified(pc_notify_t *req, int status) {
  pc_client_t *client = req->client;
  json_t *msg = (json_t *)req->data;
  json_object_set(msg, "hi", json_string("hi~~~~~"));

  // send request
  do_request(client, msg);
}

void do_notify(pc_client_t *client, json_t *msg) {
  pc_notify_init(&notify);
  notify.client = client;
  notify.data = msg;
  pc_notify(client, &notify, route, msg, on_notified);
}

// connection established callback
void on_connected(pc_connect_t* req, int status) {
  if(status == -1) {
    fprintf(stderr, "Fail to connect to server\n");
  } else {
    printf("Connection established.\n");
  }

  pc_client_t *client = req->client;

  // clean up
  free(req->address);
  free(req);

  json_t *msg = json_object();
  json_object_set(msg, "msg", json_string("hello"));

  // send notify
  do_notify(client, msg);
}

int main() {
  // create and init Pomelo client instance
  pc_client_t *client = (pc_client_t *)malloc(sizeof(pc_client_t));

  if(client == NULL) {
    fprintf(stderr, "Fail to malloc for pc_client_t.\n");
    goto error;
  }

  struct sockaddr_in *address =
      (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));

  if(address == NULL) {
    fprintf(stderr, "Fail to malloc for address.\n");
    goto error;
  }

  memset(address, 0, sizeof(struct sockaddr_in));
  address->sin_family = AF_INET;
  address->sin_port = htons(port);
  address->sin_addr.s_addr = inet_addr(ip);

  if(pc_client_init(client)) {
    fprintf(stderr, "Fail to initiate pc_client_t.\n");
    goto error;
  }

  // create connect request and connect to server
  pc_connect_t *conn_req = (pc_connect_t *)malloc(sizeof(pc_connect_t));

  if(conn_req == NULL) {
    fprintf(stderr, "Fail to malloc pc_connect_t.\n");
    goto error;
  }

  if(pc_connect_init(conn_req, address)) {
    fprintf(stderr, "Fail to initiate pc_connect_t.\n");
    goto error;
  }
  if(pc_connect(client, conn_req, NULL, on_connected)) {
    fprintf(stderr, "Fail to connect to server.\n");
    goto error;
  }

  pc_run(client);

  return 0;

error:
  if(!client) free(client);
  if(!address) free(address);
  if(!conn_req) free(conn_req);
  return -1;
}