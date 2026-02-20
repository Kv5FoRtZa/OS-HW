// SPDX-License-Identifier: BSD-3-Clause
//ss -ltnp | grep ':8888'
//daca opresc checkerul se blocheaza random ceva pe port si trebuie dat kill
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/eventfd.h>
#include <libaio.h>
#include <errno.h>
#include "aws.h"
#include "utils/util.h"
#include "utils/debug.h"
#include "utils/sock_util.h"
#include "utils/w_epoll.h"
/* server socket file descriptor */
static int listenfd;

/* epoll file descriptor */
static int epollfd;

static io_context_t ctx;

static int set_nonblocking(int fd)
{
	//vede daca e nonblocking sau nu dupa flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0){
		return -1;
	}
	return 0;

}

static int aws_on_path_cb(http_parser *p, const char *buf, size_t len)
{
	// s
	struct connection *conn = (struct connection *)p->data;
	size_t curent = strlen(conn->request_path);
	size_t l_path = sizeof(conn->request_path) - 1 - curent;
	size_t to_copy;
	if(len < l_path){
		to_copy = len;
	}
	else{
		to_copy = l_path;
	}
	memcpy(conn->request_path + curent, buf, to_copy);
	conn->request_path[curent + to_copy] = '\0';
	conn->have_path = 1;
	return 0;
}

 static void connection_prepare_send_reply_header(struct connection *conn)
{
	//la fel ca 404 dar e http ok nu eroare
	int n = snprintf(conn->send_buffer, sizeof(conn->send_buffer),"HTTP/1.1 200 OK\r\n""Content-Length: %zu\r\n""Connection: close\r\n""\r\n",conn->file_size);
	if (n < 0)
	{
		n = 0;
	}
	conn->send_pos = 0;
	conn->send_len = n;
	conn->state = STATE_SENDING_DATA;
}

static void connection_prepare_send_404(struct connection *conn) {
	// seteaza state, si baga in send_len mesajul de 404
	conn->send_pos = 0;
	int n = snprintf(conn->send_buffer, sizeof(conn->send_buffer), "HTTP/1.1 404 Not Found\r\n" "Content-Length: 0\r\n" "Connection: close\r\n" "\r\n");
	if (n < 0)
	{
		n = 0;
	}
	conn->send_len = n;
	conn->state = STATE_SENDING_404;
 }

 static enum resource_type connection_get_resource_type(struct connection *conn) {
	//vede ce tip de conexiune este
	//static sau dinamic, desi la dinamic literalmente trimit 404 mereu dar da
	char *request_path = conn->request_path;
	if (strncmp(request_path, "/static/",8) == 0) {
		conn->res_type = RESOURCE_TYPE_STATIC;
		snprintf(conn->filename, sizeof(conn->filename), "%s%s",AWS_ABS_STATIC_FOLDER, request_path + 8);
		return RESOURCE_TYPE_STATIC;
	}
	if (strncmp(request_path, "/dynamic/",9) == 0) {
		conn->res_type = RESOURCE_TYPE_DYNAMIC;
		snprintf(conn->filename, sizeof(conn->filename), "%s%s",AWS_ABS_DYNAMIC_FOLDER, request_path + 9);
		return RESOURCE_TYPE_DYNAMIC;
	}
	conn->res_type = RESOURCE_TYPE_NONE;
	return RESOURCE_TYPE_NONE;
 }

 void connection_start_async_io(struct connection *conn) {
	/* TODO: Start asynchronous operation (read from file). * Use io_submit(2) & friends for reading data asynchronously. */
}

struct connection *connection_create(int sockfd) {
	// aloca memoria pt conexiune pt structul de conn
	// seteaza -1 si niste chestii default
	struct connection *conn = calloc(1, sizeof(*conn));
	if (!conn)
	{
		return NULL;
	}
	conn->sockfd = sockfd;
	conn->fd = -1;
	conn->eventfd = -1;
	conn->state = STATE_RECEIVING_DATA;
	http_parser_init(&conn->request_parser, HTTP_REQUEST);
	conn->request_parser.data = conn;
	return conn;
}

void connection_remove(struct connection *conn) {
	//sterge, elibereaza memoria, da remove
	if (!conn){
		return;
	}
	w_epoll_remove_ptr(epollfd, conn->sockfd, conn);
	close(conn->sockfd);
	if (conn->eventfd != -1) {
		w_epoll_remove_ptr(epollfd, conn->eventfd, conn);
		close(conn->eventfd);
	}
	if (conn->fd != -1)
		close(conn->fd);
	free(conn);
}
void handle_new_connection(void)
{
	while (1) {
		// accepta, daca e eroare return, daca nu atunci se vede daca e blocnat sau nu
		//daca merge mai departe se creaza conexiunea
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int clientfd = accept(listenfd, (struct sockaddr *)&addr, &len);
		if (clientfd < 0) {
			return;
		}
		if (set_nonblocking(clientfd) < 0) {
			close(clientfd);
		}
		else{
			struct connection *conn = connection_create(clientfd);
			if (!conn) {
				close(clientfd);
			}
			else if (w_epoll_add_ptr_in(epollfd, clientfd, conn) < 0) {
				connection_remove(conn);
			}
		}
 	}
 }

void receive_data(struct connection *conn) {
	//primeste date pana cand state closed
	while (1) {
		if (conn->recv_len >= sizeof(conn->recv_buffer)) {
			conn->state = STATE_CONNECTION_CLOSED;
			return;
		}
		//n e size de date primite
		//se vede daca e mai mare decat 0 , adica inca mai primeste, sau e 0, adica nu mai primeste/ s-a terminat 
		ssize_t n = recv(conn->sockfd, conn->recv_buffer + conn->recv_len, sizeof(conn->recv_buffer) - conn->recv_len, 0);
		if (n > 0) {
			conn->recv_len += (size_t)n;
		}
		else{
			if (n == 0) {
				if (conn->recv_len > 0)
					return;
				conn->state = STATE_CONNECTION_CLOSED;
				return;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			conn->state = STATE_CONNECTION_CLOSED;
			return;
		}
	}
}

int connection_open_file(struct connection *conn)
{
	//e in filename
	//seteaza size si dile descriptor
	conn->fd = open(conn->filename, O_RDONLY);
	if(conn->fd < 0){
		return -1;
	}
	conn->file_pos = 0;
	struct stat st;
	fstat(conn->fd, &st);
	conn->file_size = st.st_size;
	return 0;
}

void connection_complete_async_io(struct connection *conn)
{
	/* TODO: Complete asynchronous operation;
	operation returns successfully. * Prepare socket for sending. */
}
int parse_header(struct connection *conn) {
	http_parser_settings settings_on_path = {
		.on_message_begin = 0, .on_header_field = 0,
		.on_header_value = 0,
		.on_path = aws_on_path_cb,
		.on_url = 0,
		.on_fragment = 0,
		.on_query_string = 0,
		.on_body = 0,
		.on_headers_complete = 0,
		.on_message_complete = 0
	};
	//executa parse heade, vede daca deja s-a facut sau nu, returneaza 1 daca are path valid/existent
	size_t already = (size_t)conn->request_parser.nread;
	if (already > conn->recv_len)
	{
		already = 0;
	}
	char *buf = conn->recv_buffer + already;
	size_t len = conn->recv_len - already;
	http_parser_execute(&conn->request_parser, &settings_on_path, buf, len);
	if(conn->have_path) {
		return 1;
	}
	return 0;
 }
 enum connection_state connection_send_static(struct connection *conn) {
	//daca nr de biti e 0 = data sent
	//daca nu e for some reason e inca sending
	while(conn->file_pos < conn->file_size)
	{
		off_t test= conn->file_pos;
		size_t ii = sendfile(conn->sockfd,conn->fd,&test,conn->file_size - conn->file_pos);
		if (ii == 0){
			return STATE_DATA_SENT;
		}
		conn->file_pos = test;
	}
	if(conn->file_pos >= conn->file_size)
	{
		return STATE_DATA_SENT;
	}
	
	return STATE_SENDING_DATA;
 	//return STATE_NO_STATE;
 }
 int connection_send_data(struct connection *conn) {
	//cat timp mai are de trimis date, trimite
	while (conn->send_pos < conn->send_len) {
		ssize_t n = send(conn->sockfd, conn->send_buffer + conn->send_pos, conn->send_len - conn->send_pos, MSG_NOSIGNAL);
		if (n > 0) {
			conn->send_pos += (size_t)n;
		}
		else {
			return -1;
		}

	}
	return conn->send_pos;
 }
 int connection_send_dynamic(struct connection *conn) {
 /* TODO: Read data asynchronously. * Returns 0 on success and -1 on error. */
 	return 0;
 }
 void handle_input(struct connection *conn) {
	//aici basically iesi daca e deja inchis for some reason
	// faci headerul
	//vezi daca e ok sau nu headerul
	if (conn->state != STATE_RECEIVING_DATA)
		return;
	receive_data(conn);
	if (conn->state == STATE_CONNECTION_CLOSED)
		return;
	if (conn->recv_len == 0)
		return;
	int prc = parse_header(conn);
	if(prc < 0){
		conn->state = STATE_CONNECTION_CLOSED;
		return;
	}
	if(prc == 0){
		return;
	}
	//nu mai trimit mereu 404 ca mai devreme
	enum resource_type rt = connection_get_resource_type(conn);
	if(rt == RESOURCE_TYPE_STATIC){
		if(connection_open_file(conn) < 0){
			connection_prepare_send_404(conn);
			//bad file = 404 pt static
		}
		else{
			//asta ar trebui sa mearga
			connection_prepare_send_reply_header(conn);
			conn->state = STATE_SENDING_HEADER;
		}
	}
	else{
		//trimit 404 pt dinamic pt ca dc sa nu trimit ceva
		connection_prepare_send_404(conn);
	}
	if (w_epoll_update_ptr_out(epollfd, conn->sockfd, conn) < 0) {
		conn->state = STATE_CONNECTION_CLOSED;
	}
	handle_output(conn);
	if (conn->state == STATE_CONNECTION_CLOSED)
		return;
 }

 void handle_output(struct connection *conn) {
	if (conn->state == STATE_SENDING_404)
	{
		//404 file not found si inchizi
		int rc = connection_send_data(conn);
		if (rc < 0) {
			conn->state = STATE_CONNECTION_CLOSED;
			return;
		}
		if (conn->send_pos == conn->send_len) {
			conn->state = STATE_CONNECTION_CLOSED;
		}
	}
	else if(conn->state == STATE_SENDING_HEADER)
	{
		//idk trimiti header gen si apoi inchizi
		int rc = connection_send_data(conn);
		if (rc < 0) {
			conn->state = STATE_CONNECTION_CLOSED;
			return;
		}
		if (conn->send_pos == conn->send_len) {
			conn->state = STATE_SENDING_DATA;
		}
	}
	if (conn->state == STATE_SENDING_DATA)
	{
		if(conn->res_type == RESOURCE_TYPE_STATIC){
			enum connection_state st = connection_send_static(conn);
			if(st == STATE_DATA_SENT){
				//daca e gata
				conn->state = STATE_CONNECTION_CLOSED;
				return;
			}
			if(st == STATE_CONNECTION_CLOSED){
				return;
			}
			conn->state = st;
		}
		else{
			//dinamic nu cred ca mai apuc sa fac 
			return;
		}
	}
	return;
 }

void handle_client(uint32_t event, struct connection *conn)
{
	//apeleaza input output sau close
	if (event & EPOLLERR) {
		connection_remove(conn);
		return;
	}
	if (event & EPOLLIN){
		handle_input(conn);
	}
		
	if (event & EPOLLOUT){
		handle_output(conn);
	}
	if (conn->state == STATE_CONNECTION_CLOSED) {
		connection_remove(conn);
		return;
	}
}
int main(void)
{
	epollfd = w_epoll_create();
	listenfd = tcp_create_listener(AWS_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
	set_nonblocking(listenfd);
	w_epoll_add_ptr_in(epollfd, listenfd, NULL);
	while (1) {
		//se asteapta pana cand apare o conexiune
		struct epoll_event rev;
		int rc = w_epoll_wait_infinite(epollfd, &rev);
		if (rc >= 0) {
			if (rev.data.ptr == NULL) {
				handle_new_connection();
			} else {
				struct connection *conn = (struct connection *)rev.data.ptr;
				handle_client(rev.events, conn);
			}
		}
	}
	return 0;
}
