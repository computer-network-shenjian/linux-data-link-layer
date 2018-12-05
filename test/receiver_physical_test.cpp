#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "receiver_physical_test.log") < 0) {
        cout << "Open log error!" << endl;
        return E_LOG_OPEN;
    }

    int server_fd = tcp_server_block();
    LOG(Debug) << "server_fd\t" << server_fd << endl;
    if (server_fd < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }

    Status val_physical_layer_recv = 0;

    // receive 12 bytes.
    char test[LEN_PKG_NODATA] = {0};
    val_physical_layer_recv = physical_layer_recv(server_fd, test, false);
    LOG(Debug) << "val_physical_layer_recv\t" << val_physical_layer_recv << endl;

    if (val_physical_layer_recv < 0) {
        LOG(Error) << "An error occured!" << endl;
        exit(-1);
    }
    else {
        LOG(Info) << "recv: " << test << " success" << endl;
    }

    // receive 1036 bytes.
    while (val_physical_layer_recv != TRANSMISSION_END) {
        char test[LEN_PKG_DATA] = {0};
        val_physical_layer_recv = physical_layer_recv(server_fd, test);
        LOG(Debug) << "val_physical_layer_recv\t" << val_physical_layer_recv << endl;
        if (val_physical_layer_recv == TRANSMISSION_END) {
            LOG(Info) << "recv: " << test << " success" << endl;
            LOG(Info) << "transmission end" << endl;
            break;
        }

        if (val_physical_layer_recv < 0) {
            LOG(Error) << "An error occured!" << endl;
            exit(-1);
        }
        else {
            LOG(Info) << "recv: " << test << " success" << endl;
        }
    }

    LOG(Info) << endl << "receiver_physical_layer test passed!" << endl;
    log_stream.close();
    return 0;
} 
