#include "../common/shared_library.hpp"

int main() {
	 std::ofstream log_stream;
    if(log_init(log_stream, "receiver1.log") < 0) {
        cout << "Open log error!" << endl;
        return E_LOG_OPEN;
    }
    
    LOG(Info) << "RNL start" << endl;

    int pipe_network_datalink[2];
    if (pipe(pipe_network_datalink) < 0) {
        LOG(Error) << "pipe_network_datalink init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "pipe_network_datalink init ok" << endl;
    }

    signal(SIGCHLD, SIG_IGN);

    pid_t datalink_pid = fork();

    if (datalink_pid < 0) {
        LOG(Error) << "fork unsuccessful" << endl;
        return E_FORK;
    }
    else if (datalink_pid == 0) {
        LOG(Info) << "RDL start" << endl;
        Status val_datalink = receiver_datalink_layer(utopia, pipe_network_datalink);
        if (val_datalink < 0) {
            LOG(Error) << "Error occured in RDL with code: " << val_datalink << endl;
            return val_datalink;
        }
        else {
            LOG(Info) << "RDL end with success" << endl;
            return ALL_GOOD;
        }
    }
    else {
        close(pipe_network_datalink[p_write]);   // write only
        char pipe_buf[RAW_DATA_SIZE + 1] = {0};
        if (read(pipe_network_datalink[p_read], pipe_buf, RAW_DATA_SIZE) < 0) {
            LOG(Error) << "Pipe read from RDL to RNL error" << endl;
            return E_PIPE_READ;
        }
        LOG(Info) << "RNL read from RDL: " << pipe_buf << endl;
        sleep(1);
        close(pipe_network_datalink[p_read]);
        LOG(Info) << "RNL end with success" << endl;
        log_stream.close();
        return ALL_GOOD;
    }
    return 0;
}