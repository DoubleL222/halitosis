#include "bot/math.hpp"

int ceil_div(int a, int b) {
    return (a+b-1)/b;
}

int pos_mod(int a, int b) {
    return (a%b+b)%b;
}
