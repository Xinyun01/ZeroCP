// Copyright (c) 2025 ZeroCopy Framework Team. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef ZEROCP_VOCABULARY_MACROS_HPP
#define ZEROCP_VOCABULARY_MACROS_HPP

#include <cassert>

/// @brief 丢弃函数返回值的宏，用于抑制"未使用返回值"的编译器警告
/// @param[in] x 要调用的函数表达式
/// @note 使用方式：ZEROCP_DISCARD_RESULT(function_call());
/// @note 这个宏将函数返回值转换为 void，明确表示有意忽略返回值
/// 
/// @details 当函数返回值被标记为 [[nodiscard]] 但确实不需要检查返回值时使用此宏。
///          例如，在已知操作必然成功的上下文中（如容量已检查的 vector 操作）。
#ifndef ZEROCP_DISCARD_RESULT
#define ZEROCP_DISCARD_RESULT(x) ((void)(x))
#endif


/// @brief 运行时断言宏，用于检查前提条件
/// @param[in] condition 要检查的条件表达式
/// @param[in] message 断言失败时的错误消息
/// @note 在调试模式下，如果条件为 false，将触发断言并输出消息
/// @note 在发布模式下，断言可能被优化掉
/// 
/// @details 用于检查程序的不变量和前提条件。如果条件不满足，
///          表示程序存在严重的逻辑错误，应该立即终止。
#ifndef ZEROCP_ENFORCE
#define ZEROCP_ENFORCE(condition, message) \
    do { \
        if (!(condition)) { \
            assert((condition) && (message)); \
        } \
    } while(0)
#endif


#endif // ZEROCP_VOCABULARY_MACROS_HPP

