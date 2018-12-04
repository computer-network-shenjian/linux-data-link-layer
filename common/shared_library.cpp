#include "shared_library.hpp"

int tcp_server_block(const int port) {
    // AF_INET: IPv4 protocol
	// SOCK_STREAM: TCP protocol
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) { 
		graceful_return("socket", E_CREATE_SOCKET);
	} 
	
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
		graceful_return("setsockopt", E_SETSOCKOPT);
	}

	struct sockaddr_in server_addr; 
	server_addr.sin_family = AF_INET; 
	// INADDR_ANY means 0.0.0.0(localhost), or all IP of local machine.
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port); 
	int server_addrlen = sizeof(server_addr);
	if (bind(server_fd, (struct sockaddr *) &server_addr, server_addrlen) < 0) { 
		graceful_return("bind", E_BIND);
	}

	if (listen(server_fd, TCP_LISTEN_NUM) < 0) { 
		graceful_return("listen", E_LISTEN); 
	}

	int new_socket = accept(server_fd, (struct sockaddr *)&server_addr, (socklen_t*) &server_addrlen);
	if (new_socket < 0) { 
		graceful_return("accept", E_ACCEPT); 
	}
    else {
        LOG(Info) << "server accept client success" << endl;
    }

    return new_socket;
}

int tcp_client_block(const char *ip, const int port) {
	// AF_INET_IPv4 protocol
	// SOCK_STREAM: TCP protocol
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) { 
		graceful_return("socket", E_CREATE_SOCKET);
	}

	struct sockaddr_in server_addr; 
	memset(&server_addr, '0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); 
	if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
		LOG(Error) << "wrong peer IP" << endl;
        graceful_return("wrong peer IP", E_WRONG_IP);
	} 

	if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { 
		graceful_return("connect", E_CONNECT); 
	}
	else {
		LOG(Info) << "client connect server success" << endl;
	}

	return client_fd;
}

Status physical_layer_send(const int socket, const char *buf_send, const bool is_data, const bool is_end) {
    bool is_data_confirm = is_data | is_end;    // if is_end == true, is_data_confirm must be true.
    const unsigned int buf_length = is_data_confirm ? LEN_PKG_DATA : LEN_PKG_NODATA;
    char buffer[LEN_PKG_DATA] = {0};
    if (!is_end) {
        memcpy(buffer, buf_send, buf_length);
    }
    unsigned int total_send = 0;
    while (total_send < buf_length) {
        int val_send = send(socket, buffer, buf_length, 0);
        if (val_send < 0) {
            graceful_return("send", E_SEND);
        }
        else if (val_send == 0) {
            graceful_return("peer disconnected", E_PEER_DISCONNECTED);
        }
        else {
            total_send += val_send;
        }
    }
    
    if (total_send > buf_length) {
        LOG(Error) << "wrong byte sent" << endl;
        return E_WRONG_BYTE;
    }
    else {
        return ALL_GOOD;
    }
}

Status physical_layer_recv(const int socket, char *buf_recv, const bool is_data) {
    const unsigned int buf_length = is_data ? LEN_PKG_DATA : LEN_PKG_NODATA;
    char buffer[LEN_PKG_DATA] = {0};
    unsigned int total_recv = 0;
    while (total_recv < buf_length) {
        int val_recv = recv(socket, buffer, buf_length, 0);
        if (val_recv < 0) {
            graceful_return("recv", E_RECV);
        }
        else if (val_recv == 0) {
            graceful_return("peer disconnected", E_PEER_DISCONNECTED);
        }
        else {
            total_recv += val_recv;
        }
    }
    
    if (total_recv > buf_length) {
        LOG(Error) << "wrong byte sent" << endl;
        return E_WRONG_BYTE;
    }
    // TODO: better way to check 1036 consecutive bytes from buffer is all '\0'.
    memcpy(buf_recv, buffer, buf_length);
    /*
    if (strlen(buffer) == 0) {
        return TRANSMISSION_END;
    }
    */
    if (0 == memcmp(all_zero, buffer, LEN_PKG_DATA)) {
        return TRANSMISSION_END;
    }
    else {
        return ALL_GOOD;
    }
}

Status sender_datalink_layer(DProtocol protocol, int *pipe) {
    Status val;
    switch(protocol) {
        case(test): {
            LOG(Info) << "Getting into SDL with protocol: " << "test" << endl;
            val = sender_datalink_layer_test(pipe);
            LOG(Debug) << "Return value of sender_datalink_layer_test\t" << val << endl;
            return val;
        }
        default: {
            LOG(Error) << "Datalink protocol selection error" << endl;
            return E_DATALINK_SELECT;
        }
    }
}

Status sender_datalink_layer_test(int *pipe) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    close(pipe[p_write]);    // read only
    char pipe_buf[20];
    if (read(pipe[p_read], pipe_buf, 20) <= 0) {
        LOG(Error) << "Pipe read from SNL to SDL error" << endl;
        return E_PIPE_READ;
    }
    LOG(Info) << "Get info from SNL: " << pipe_buf << endl;
    close(pipe[p_read]);
    LOG(Info) << "SDL test passed!" << endl;
    return ALL_GOOD;
}

Status log_init(std::ofstream &log_stream, const std::string log_name, const Level level) {
    // log_stream must not be opened before getting into this function.
    if (log_stream.is_open()) {
        return E_LOG_OPEN;
    }
    log_stream.open(log_name, ios::out|ios::trunc);
    if (!log_stream.is_open()) {
        return E_LOG_OPEN;
    }
    Log::get().setLogStream(log_stream);
    Log::get().setLevel(level);
    return ALL_GOOD;
}
