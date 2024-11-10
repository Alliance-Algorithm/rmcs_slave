/**
 * @author Qzh (zihanqin2048@gmail.com)
 * @brief The assertion error handling.
 * @copyright Copyright (c) 2023 by Alliance, All Rights Reserved.
 */

#include <cassert>

#include <main.h>

#include "app/led/led.hpp"

const char* assert_file       = nullptr;
int assert_line               = 0;
const char* assert_function   = nullptr;
const char* assert_expression = nullptr;

void __assert_func(const char* file, int line, const char* function, const char* expression) {
    __disable_irq();

    assert_file       = file;
    assert_line       = line;
    assert_function   = function;
    assert_expression = expression;

    led::led->set_value(255, 0, 0);

    while (true)
        __NOP();
}