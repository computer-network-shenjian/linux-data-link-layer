#include<iostream>

using namespace std;

unsigned int count_ending_zeros(const char * const data, unsigned int data_length = 1024) {
    // count the number of ending zeros of an array from position data_length
    int counter = data_length;
    while (counter >= 0 && data[--counter] == 0) {};
    return data_length - 1 - counter;
}

int main() {
    char one[5] {1,1,1,1,0};
    char two[5] {1,1,1,0,0};
    char three[5] {1,1,0,0,0};
    char zero[5] {};
    cout << "one: " << count_ending_zeros(one, 5) << endl
        << "two: " << count_ending_zeros(two, 5) << endl
        << "three: " << count_ending_zeros(three, 5) << endl
        << "zero: " << count_ending_zeros(zero, 5) << endl;
}
