#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "sender_physical_test.log") < 0) {
        cout << "Open log error!" << endl;
        return E_LOG_OPEN;
    }

    int client_fd = tcp_client_block();
    LOG(Debug) << "client_fd\t" << client_fd << endl;
    if (client_fd < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }

    int flags;
    flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    char test1[LEN_PKG_NODATA+1] = "000100020003";
    char test2[LEN_PKG_DATA] = "000100020003Hello receiver!";
    char test3[LEN_PKG_DATA] = "000100020003Hello again!";
    char test4[LEN_PKG_DATA] = "This should not be received!";

    Status val_tcp_send;

    val_tcp_send = tcp_send(client_fd, test1, false);
    LOG(Debug) << "val_tcp_send\t" << val_tcp_send << endl;
    if (val_tcp_send < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "send test1: " << test1 << " success" << endl;
    }

    val_tcp_send = tcp_send(client_fd, test2);
    LOG(Debug) << "val_tcp_send\t" << val_tcp_send << endl;
    if (val_tcp_send < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "send test2: " << test2 << " success" << endl;
    }

    val_tcp_send = tcp_send(client_fd, test3);
    LOG(Debug) << "val_tcp_send\t" << val_tcp_send << endl;
    if (val_tcp_send < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "send test3: " << test3 << " success" << endl;
    }

    val_tcp_send = tcp_send(client_fd, test3, true, true);
    LOG(Debug) << "val_tcp_send\t" << val_tcp_send << endl;
    if (val_tcp_send < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "send end success" << endl;
    }

    val_tcp_send = tcp_send(client_fd, test4);
    LOG(Debug) << "val_tcp_send\t" << val_tcp_send << endl;
    if (val_tcp_send < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "send test4: " << test4 << " success, and this should not be received!" << endl;
    }

    LOG(Info) << endl << "sender_physical_layer test passed!" << endl;
    log_stream.close();
    return 0;
} 
