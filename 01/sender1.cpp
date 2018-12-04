#include "../common/shared_library.hpp"

using namespace fly;

int main() {
    Log::get().setLogStream(std::cout);
    Log::get().setLevel(Debug);
    LOG(Info) << "hello info!" << endl;
    LOG(Debug) << "hello debug!" << endl;
    LOG(Error) << "hello error!" << endl;
    return 0;
}