//
// Created by Yinhao on 4/9/2022.
//

#ifndef CODE_INCLUDE_H
#define CODE_INCLUDE_H

#include <cstdint>
#include <tuple>
#include <array>
#include <cmath>
#include <exception>
#include <chrono>
#include <bitset>
#include <vector>
#include <fstream>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>
#include <forward_list>

enum class task_t {
	task_readAddress, task_writeAddress, task_reportHitMiss, task_reportImage, task_halt
};


#define POLICY_WBWA false
#define POLICY_WTNWA true

#define C_HIT true
#define C_MISS false


#endif //CODE_INCLUDE_H
