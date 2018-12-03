#include "../common/shared_library.hpp"

int main() {
    int server_fd = tcp_server_block();
    cout << "DEBUG: server_fd\t" << server_fd << endl;
    if (server_fd < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }

    Status val_physical_layer_recv = 0;

    // receive 12 bytes.
    char test[LEN_PKG_NODATA] = {0};
    val_physical_layer_recv = physical_layer_recv(server_fd, test, false);
    cout << "DEBUG: val_physical_layer_recv\t" << val_physical_layer_recv << endl;

    if (val_physical_layer_recv < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "recv: " << test << " success" << endl;
    }

    // receive 1036 bytes.
    while (val_physical_layer_recv != TRANSMISSION_END) {
        char test[LEN_PKG_DATA] = {0};
        val_physical_layer_recv = physical_layer_recv(server_fd, test);
        cout << "DEBUG: val_physical_layer_recv\t" << val_physical_layer_recv << endl;
        if (val_physical_layer_recv == TRANSMISSION_END) {
            cout << "DEBUG: transmission end" << endl;
            break;
        }

        if (val_physical_layer_recv < 0) {
            cout << "DEBUG: An error occured!" << endl;
            exit(-1);
        }
        else {
            cout << "recv: " << test << " success" << endl;
        }
    }

    cout << endl << "receiver_physical_layer test passed!" << endl;
    return 0;
} 
