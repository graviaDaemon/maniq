#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const char maniq_source[] = "v0-mq70001.mq";

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
void mq_main();

int32_t mq_max(int32_t mq_a, int32_t mq_b) {
    if (mq_a > mq_b) {
        return mq_a;
    }
    return mq_b;
}

void mq_main() {
    mq_max(10, 15);
    maniq_print_int(20);
}

int main() {
    mq_main();
    return 0;
}
