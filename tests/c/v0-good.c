#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const char maniq_source[] = "v0-good.mq";

[[noreturn]] void maniq_runtime_error(const char* code, const char* message, int line, int col) {
    fflush(stdout);
    fprintf(stderr, "%s:%d:%d\n    ==> Runtime-Error[%s]: %s\n", maniq_source, line, col, code, message);
    exit(1);
}

void maniq_print_int(int32_t value) {
    printf("%" PRId32 "\n", value);
}

void maniq_print_bool(bool value) {
    puts(value ? "true" : "false");
}

int32_t maniq_div(int32_t left, int32_t right, int line, int col) {
    if (right == 0) {
        maniq_runtime_error("MQ60001", "division by zero", line, col);
    }
    if (right == -1) {
        return (int32_t)(0u - (uint32_t)left);
    }
    return left / right;
}

int32_t maniq_mod(int32_t left, int32_t right, int line, int col) {
    if (right == 0) {
        maniq_runtime_error("MQ60001", "division by zero", line, col);
    }
    if (right == -1) {
        return 0;
    }
    return left % right;
}

int32_t mq_max(int32_t mq_a, int32_t mq_b);
bool mq_isEven(int32_t mq_n);
void mq_main();
void mq_countDown(int32_t mq_start);

int32_t mq_max(int32_t mq_a, int32_t mq_b) {
    if (mq_a > mq_b) {
        return mq_a;
    }
    return mq_b;
}

bool mq_isEven(int32_t mq_n) {
    return maniq_mod(mq_n, 2, 9, 14) == 0;
}

void mq_main() {
    mq_countDown(3);
    int32_t mq_biggest = mq_max(3, 7);
    maniq_print_int(mq_biggest);
    bool mq_even = mq_isEven(mq_biggest);
    if (mq_even && (mq_biggest > 5)) {
        maniq_print_bool(true);
    } else if (!mq_even || (mq_biggest <= 0)) {
        maniq_print_bool(false);
    } else {
        maniq_print_int(0);
    }
    int32_t mq_negative = -mq_biggest;
    maniq_print_int((((2 + 3) * 4) - maniq_div(6, 2, 28, 27)) + mq_negative);
}

void mq_countDown(int32_t mq_start) {
    int32_t mq_count = mq_start;
    while (mq_count > 0) {
        maniq_print_int(mq_count);
        mq_count = mq_count - 1;
    }
}

int main() {
    mq_main();
    return 0;
}
