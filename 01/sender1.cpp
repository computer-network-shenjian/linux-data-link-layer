#include "../common/shared_library.hpp"

int main() {
    std::ofstream log_stream;
    if(log_init(log_stream, "sender1.log") < 0) {
        cout << "Open log error!" << endl;
        return E_LOG_OPEN;
    }
    
    log_stream.close();
    return 0;
}
