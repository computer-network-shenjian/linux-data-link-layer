#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "sender2.log", Info) < 0) {
        cout << "[SNL] Open log error!" << endl;
        return E_LOG_OPEN;
    }
    
    LOG(Info) << "[SNL] SNL start" << endl;

    int pipe_network_datalink[2];
    if (pipe(pipe_network_datalink) < 0) {
        LOG(Error) << "[SNL] pipe_network_datalink init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[SNL] pipe_network_datalink init ok" << endl;
    }

    //signal(SIGCHLD, SIG_IGN);

    pid_t datalink_pid = fork();

    if (datalink_pid < 0) {
        LOG(Error) << "[SNL] fork unsuccessful" << endl;
        return E_FORK;
    }
    else if (datalink_pid == 0) {
        //Status val_datalink = sender_datalink_layer(test, pipe_network_datalink);
        //Status val_datalink = sender_datalink_layer(utopia, pipe_network_datalink);
        Status val_datalink = sender_datalink_layer(simple_stop_and_wait, pipe_network_datalink);
        if (val_datalink < 0) {
            LOG(Error) << "[SDL] Error occured in SDL with code: " << val_datalink << endl;
            return val_datalink;
        }
        else {
            LOG(Info) << "[SDL] SDL end with success" << endl;
            return ALL_GOOD;
        }
    }
    else {
        Status val_snl = sender_network_layer(pipe_network_datalink, datalink_pid);
        LOG(Debug) << "[SNL] val_snl\t" << val_snl << endl;
        if (val_snl < 0) {
            LOG(Error) << "[SNL] Error occured in SNL with code: " << val_snl << endl;
            log_stream.close();
            return val_snl;
        }
        else {
            LOG(Info) << "[SNL] SNL end with success" << endl;
            log_stream.close();
            return ALL_GOOD;
        }
    }
}
