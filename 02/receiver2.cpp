#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "receiver2.log", Info) < 0) {
        cout << "[RNL] Open log error!" << endl;
        return E_LOG_OPEN;
    }
    
    LOG(Info) << "[RNL] RNL start" << endl;

    int pipe_network_datalink[2];
    if (pipe(pipe_network_datalink) < 0) {
        LOG(Error) << "[RNL] pipe_network_datalink init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[RNL] pipe_network_datalink init ok" << endl;
    }

    signal(SIGCHLD, SIG_IGN);

    pid_t datalink_pid = fork();

    if (datalink_pid < 0) {
        LOG(Error) << "[RNL] fork unsuccessful" << endl;
        return E_FORK;
    }
    else if (datalink_pid == 0) {
        Status val_datalink = receiver_datalink_layer(simple_stop_and_wait, pipe_network_datalink);
        if (val_datalink < 0) {
            LOG(Error) << "[RDL] Error occured in RDL with code: " << val_datalink << endl;
            return val_datalink;
        }
        else {
            LOG(Info) << "[RDL] RDL end with success" << endl;
            return ALL_GOOD;
        }
        
    }
    else {
        Status val_rnl = receiver_network_layer(pipe_network_datalink);
        LOG(Debug) << "[RNL] val_rnl\t" << val_rnl << endl;
        if (val_rnl < 0) {
            LOG(Error) << "[RNL] Error occured in RNL with code: " << val_rnl << endl;
            log_stream.close();
            return val_rnl;
        }
        else {
            LOG(Info) << "[RNL] RNL end with success" << endl;
            log_stream.close();
            return ALL_GOOD;
        }
    }
    return 0;
}
