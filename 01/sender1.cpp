#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "sender1.log") < 0) {
        cout << "Open log error!" << endl;
        return E_LOG_OPEN;
    }
    
    LOG(Info) << "SNL start" << endl;

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
        LOG(Info) << "SDL start" << endl;
        Status val_datalink = sender_datalink_layer(utopia, pipe_network_datalink);
        if (val_datalink < 0) {
            LOG(Error) << "Error occured in SDL with code: " << val_datalink << endl;
            return val_datalink;
        }
        else {
            LOG(Info) << "SDL end with success" << endl;
            return ALL_GOOD;
        }
    }
    else {
        close(pipe_network_datalink[p_read]);   // write only
        char pipe_buf[20] = "hello SDL";
        if (write(pipe_network_datalink[p_write], pipe_buf, 20) < 0) {
            LOG(Error) << "Pipe write from SNL to SDL error" << endl;
            return E_PIPE_WRITE;
        }
        LOG(Info) << "SNL sent to SDL: " << pipe_buf << endl;
        sleep(1);
        //int val_waitpid = waitpid(datalink_pid, NULL, 0);
        //LOG(Debug) << "SNL: val_waitpid\t" << val_waitpid << endl;
        close(pipe_network_datalink[p_write]);
        LOG(Info) << "SNL end with success" << endl;
        log_stream.close();
        return ALL_GOOD;
    }
}
