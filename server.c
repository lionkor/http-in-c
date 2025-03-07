#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const char* CRLF = "\r\n";
const char* SP = " ";

typedef struct {
    const char* data;
    size_t len;
} string;

bool string_equal(string* l, string* r) {
    return l->len == r->len && memcmp(l->data, r->data, l->len) == 0;
}

string string_from_cstr(const char* str) {
    string s;
    s.len = strlen(str);
    s.data = str;
    return s;
}

void string_trim_spaces(string* s) {
    /* left trim */
    while (*s->data == ' ') {
        s->data += 1;
    }
    /* right trim */
    while (s->len > 0 && s->data[s->len-1] == ' ') {
        s->len -= 1;
    }
}

/* Request-Line = Method SP Request-URI SP HTTP-Version CRLF */
typedef struct {
    string method;
    string uri;
    string version;
} http_req_line;

typedef enum {
    HTTP_RES_OK = 200,
    HTTP_RES_BAD_REQUEST = 400,
    HTTP_RES_INTERNAL_SERVER_ERR = 500,
} http_status;

typedef struct {
    const char* start;
    size_t len;
} string_view;

typedef struct {
    string_view* splits;
    size_t count;
    size_t capacity;
} string_splits;

static string_splits split_string(const char* str, size_t len, const char* split_by) {
    string_splits result;
    const char* start = str;
    size_t result_i = 0;
    size_t split_by_len = strlen(split_by);

    result.capacity = 8;
    result.splits = calloc(sizeof(string_view), result.capacity);
    result.count = 0;

    for (size_t i = 0; i < len; ++i) {
        if (i + split_by_len < len && memcmp(&str[i], split_by, split_by_len) == 0) {
            result.splits[result_i].start = start;
            result.splits[result_i].len = &str[i] - start;
            result.count += 1;
            result_i += 1;
            start = &str[i + split_by_len];
            i += split_by_len;

            if (result.count == result.capacity) {
                result.capacity *= 2;
                string_view* temp = realloc(result.splits, sizeof(string_view) * result.capacity);
                if (temp) {
                    result.splits = temp;
                } else {
                    perror("realloc()");
                    abort();
                }
            }
        }
    }
    /* we might miss the last one if the input string doesn't end with the delimiter */
    size_t last_len = &str[len] - start;
    if (last_len > 0) {
        result.splits[result_i].start = start;
        result.splits[result_i].len = last_len;
        result.count += 1;
    }
    return result;
}

static void free_splits(string_splits* splits) {
    if (splits) {
        free(splits->splits);
        splits->splits = NULL;
    }
}

http_req_line http_req_line_init() {
    http_req_line line;
    memset(&line, 0, sizeof(line));
    return line;
}

http_status parse_req_line(http_req_line* req_line, const char* buf, size_t len) {
    if (!buf || !req_line) {
        return HTTP_RES_INTERNAL_SERVER_ERR;
    }

    string_splits components = split_string(buf, len, SP);

    if (components.count != 3) {
        printf("ERROR: invalid request line: expected 3 components, got %zu\n", components.count);
        return HTTP_RES_BAD_REQUEST;
    }

    req_line->method.data = components.splits[0].start;
    req_line->method.len = components.splits[0].len;
    req_line->uri.data = components.splits[1].start;
    req_line->uri.len = components.splits[1].len;
    req_line->version.data = components.splits[2].start;
    req_line->version.len = components.splits[2].len;

    free_splits(&components);
    return HTTP_RES_OK;
}

int handle_client(int client_socket) {
    ssize_t n = 0;
    char buf[1024];
    const char* hello = "HTTP/1.0 200 OK\r\n\r\n<h1>Hello, World!</h1>";
    const char* bye = "HTTP/1.0 200 OK\r\n\r\n<h1>Bye, World!</h1>";

    printf("\n---\n");
    for (;;) {
        memset(buf, 0, sizeof(buf));

        n = read(client_socket, buf, sizeof(buf) - 1);
        if (n < 0) {
            perror("read(client)");
            return -1;
        }
        if (n == 0) {
            printf("connection closed gracefully!\n");
            break;
        }

        printf("REQUEST:\n%s", buf);

        string_splits lines = split_string(buf, n, CRLF);

        if (lines.count < 1) {
            printf("ERROR: empty request\n");
            return -1;
        }

        http_req_line req_line = http_req_line_init();
        http_status result = parse_req_line(&req_line, lines.splits[0].start, lines.splits[0].len);
        if (result != HTTP_RES_OK) {
            /* TODO: Return correct error + error page */
            printf("ERROR: failed to parse request line\n");
            return -1;
        }

        string route_hello = string_from_cstr("/hello");
        string route_bye = string_from_cstr("/bye");

        if (string_equal(&req_line.uri, &route_hello)) {
            (void)write(client_socket, hello, strlen(hello));
        } else if (string_equal(&req_line.uri, &route_bye)) {
            (void)write(client_socket, bye, strlen(bye));
        } else {
            printf("ERROR: unknown route: \"%.*s\"\n", (int)req_line.uri.len, req_line.uri.data);
            return -1;
        }

        free_splits(&lines);
        close(client_socket);
        break;
    }
    printf("\n---\n");

    return 0;
}

int main(void) {
    /* declare */
    int rc = 0;
    struct sockaddr_in bind_addr;
    int tcp_socket = 0;
    int ret = 0;
    int client_socket = 0;
    int enabled = true;

    /* initialize */
    memset(&bind_addr, 0, sizeof(bind_addr));
    tcp_socket = socket(
        AF_INET, /* IPv4 */
        SOCK_STREAM, /* TCP */
        0 /* dont care */
    );

    if (tcp_socket < 0) {
        perror("socket()");
        return 1;
    }
    printf("socket creation succeeded\n");

    /* we dont really care if this fails */
    (void)setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    bind_addr.sin_port = htons(6969);
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(tcp_socket, (const struct sockaddr*)&bind_addr, sizeof(bind_addr));
    if (rc < 0) {
        perror("bind()");
        ret = 1;
        goto exit;
    }
    printf("bind succeeded\n");

    rc = listen(tcp_socket, SOMAXCONN);
    if (rc < 0) {
        perror("listen()");
        ret = 1;
        goto exit;
    }
    printf("listen succeeded\n");

    for (;;) {
        printf("waiting for connections...\n");
        client_socket = accept(tcp_socket, NULL, NULL);

        printf("got a connection!\n");
        rc = handle_client(client_socket);
        /* ignore errors, dont care for now */
    }

exit:
    close(tcp_socket);
    return ret;
}
