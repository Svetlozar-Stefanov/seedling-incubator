
typedef enum {
    SERVER_SUCCESS = 0,
    SERVER_ERR_INITIALIZATION_FAILED
} server_err_t;
const char* getServerErrorMessage(server_err_t err);

server_err_t startServer();