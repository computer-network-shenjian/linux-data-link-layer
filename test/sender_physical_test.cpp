#include "../common/shared_library.hpp"

int main() {
    int client_fd = tcp_client_block();
    cout << "DEBUG: client_fd\t" << client_fd << endl;
    if (client_fd < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }

    char test1[LEN_PKG_NODATA+1] = "000100020003";
    char test2[LEN_PKG_DATA] = "000100020003Hello receiver!";
    char test3[LEN_PKG_DATA] = "000100020003Hello again!";
    char test4[LEN_PKG_DATA] = "This should not be received!";

    Status val_physical_layer_send;

    val_physical_layer_send = physical_layer_send(client_fd, test1, false);
    cout << "DEBUG: val_physical_layer_send\t" << val_physical_layer_send << endl;
    if (val_physical_layer_send < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "send test1: " << test1 << " success" << endl;
    }

    val_physical_layer_send = physical_layer_send(client_fd, test2);
    cout << "DEBUG: val_physical_layer_send\t" << val_physical_layer_send << endl;
    if (val_physical_layer_send < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "send test2: " << test2 << " success" << endl;
    }

    val_physical_layer_send = physical_layer_send(client_fd, test3);
    cout << "DEBUG: val_physical_layer_send\t" << val_physical_layer_send << endl;
    if (val_physical_layer_send < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "send test3: " << test3 << " success" << endl;
    }

    val_physical_layer_send = physical_layer_send(client_fd, NULL, true, true);
    cout << "DEBUG: val_physical_layer_send\t" << val_physical_layer_send << endl;
    if (val_physical_layer_send < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "send end success" << endl;
    }

    val_physical_layer_send = physical_layer_send(client_fd, test4);
    cout << "DEBUG: val_physical_layer_send\t" << val_physical_layer_send << endl;
    if (val_physical_layer_send < 0) {
        cout << "DEBUG: An error occured!" << endl;
        exit(-1);
    }
    else {
        cout << "send test4: " << test4 << " success, and this should not be received!" << endl;
    }

    cout << endl << "sender_physical_layer test passed!" << endl;
    return 0;
} 
