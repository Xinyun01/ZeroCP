// Copyright (c) 2023 by ekxide IO GmbH. All rights reserved.
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

#ifndef ZEROCP_FOUNDATION_VOCAB_FIXED_POSITION_CONTAINER_INL
#define ZEROCP_FOUNDATION_VOCAB_FIXED_POSITION_CONTAINER_INL

#include "../include/fixed_position_container.hpp"

namespace ZeroCP
{

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>::FixedPositionContainer() noexcept
{
    initializeFreeList();
}

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>::~FixedPositionContainer() noexcept
{
    clear();
}

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>::FixedPositionContainer(const FixedPositionContainer& rhs) noexcept(
    std::is_nothrow_copy_constructible_v<T>)
{
    initializeFreeList();
    for (auto it = rhs.cbegin(); it != rhs.cend(); ++it)
    {
        emplace(*it);
    }
}

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>::FixedPositionContainer(FixedPositionContainer&& rhs) noexcept(
    std::is_nothrow_move_constructible_v<T>)
{
    initializeFreeList();
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
    {
        emplace(std::move(*it));
    }
    rhs.clear();
}

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>&
FixedPositionContainer<T, CAPACITY>::operator=(const FixedPositionContainer& rhs) noexcept(
    std::is_nothrow_copy_constructible_v<T>)
{
    if (this != &rhs)
    {
        clear();
        for (auto it = rhs.cbegin(); it != rhs.cend(); ++it)
        {
            emplace(*it);
        }
    }
    return *this;
}

template <typename T, uint64_t CAPACITY>
FixedPositionContainer<T, CAPACITY>&
FixedPositionContainer<T, CAPACITY>::operator=(FixedPositionContainer&& rhs) noexcept(
    std::is_nothrow_move_constructible_v<T>)
{
    if (this != &rhs)
    {
        clear();
        for (auto it = rhs.begin(); it != rhs.end(); ++it)
        {
            emplace(std::move(*it));
        }
        rhs.clear();
    }
    return *this;
}

template <typename T, uint64_t CAPACITY>
void FixedPositionContainer<T, CAPACITY>::clear() noexcept
{
    IndexType current = m_begin_used;
    while (current != Index::INVALID)
    {
        const IndexType next = m_nextUsed[current];
        destroyElement(current);
        releaseSlot(current);
        current = next;
    }

    m_begin_used = Index::INVALID;
    m_size = 0;
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::insert(const T& data) noexcept(std::is_nothrow_copy_constructible_v<T>)
{
    return emplaceImpl(data);
}

template <typename T, uint64_t CAPACITY>
template <typename... Targs>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::emplace(Targs&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Targs&&...>)
{
    return emplaceImpl(std::forward<Targs>(args)...);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::erase(const IndexType index) noexcept
{
    IOX_ENFORCE(index <= Index::LAST, "Index out of range!");
    IOX_ENFORCE(m_status[index] == SlotStatus::USED, "Cannot erase an empty slot!");

    const IndexType next = removeUsedSlot(index);
    destroyElement(index);
    releaseSlot(index);
    --m_size;
    return iteratorFromIndex(next);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::erase(const T* ptr) noexcept
{
    const IndexType index = findIndexFromPointer(ptr);
    IOX_ENFORCE(index != Index::INVALID, "Pointer does not belong to this container!");
    return erase(index);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::erase(Iterator it) noexcept
{
    IOX_ENFORCE(it.origins_from(*this), "Iterator does not originate from this container!");
    IOX_ENFORCE(it.to_index() != Index::INVALID, "Cannot erase end iterator!");
    return erase(it.to_index());
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::erase(ConstIterator it) noexcept
{
    IOX_ENFORCE(it.origins_from(*this), "Iterator does not originate from this container!");
    IOX_ENFORCE(it.to_index() != Index::INVALID, "Cannot erase end iterator!");
    return erase(it.to_index());
}

template <typename T, uint64_t CAPACITY>
bool FixedPositionContainer<T, CAPACITY>::empty() const noexcept
{
    return m_size == 0U;
}

template <typename T, uint64_t CAPACITY>
bool FixedPositionContainer<T, CAPACITY>::full() const noexcept
{
    return m_begin_free == Index::INVALID;
}

template <typename T, uint64_t CAPACITY>
uint64_t FixedPositionContainer<T, CAPACITY>::size() const noexcept
{
    return static_cast<uint64_t>(m_size);
}

template <typename T, uint64_t CAPACITY>
constexpr uint64_t FixedPositionContainer<T, CAPACITY>::capacity() const noexcept
{
    return CAPACITY;
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::iter_from_index(const IndexType index)
{
    return iteratorFromIndex(index);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::iter_from_index(const IndexType index) const
{
    return iteratorFromIndex(index);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator FixedPositionContainer<T, CAPACITY>::begin() noexcept
{
    return iteratorFromIndex(m_begin_used);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::begin() const noexcept
{
    return iteratorFromIndex(m_begin_used);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::cbegin() const noexcept
{
    return iteratorFromIndex(m_begin_used);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator FixedPositionContainer<T, CAPACITY>::end() noexcept
{
    return iteratorFromIndex(Index::INVALID);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::end() const noexcept
{
    return iteratorFromIndex(Index::INVALID);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::cend() const noexcept
{
    return iteratorFromIndex(Index::INVALID);
}

template <typename T, uint64_t CAPACITY>
template <typename... Args>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::emplaceImpl(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args&&...>)
{
    if (full())
    {
        return end();
    }

    const IndexType slot = acquireSlot();
    T* destination = elementAt(slot);
    std::construct_at(destination, std::forward<Args>(args)...);
    addUsedSlot(slot);
    ++m_size;
    return iteratorFromIndex(slot);
}

template <typename T, uint64_t CAPACITY>
void FixedPositionContainer<T, CAPACITY>::initializeFreeList() noexcept
{
    for (IndexType index = Index::FIRST; index <= Index::LAST; ++index)
    {
        m_status[index] = SlotStatus::FREE;
        m_nextUsed[index] = Index::INVALID;
        m_nextFree[index] = (index < Index::LAST) ? static_cast<IndexType>(index + 1) : Index::INVALID;
    }

    m_size = 0;
    m_begin_used = Index::INVALID;
    m_begin_free = Index::FIRST;
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::Iterator
FixedPositionContainer<T, CAPACITY>::iteratorFromIndex(IndexType index) noexcept
{
    if (index == Index::INVALID)
    {
        return Iterator(index, *this);
    }

    if (index > Index::LAST)
    {
        return Iterator(Index::INVALID, *this);
    }

    if (m_status[index] != SlotStatus::USED)
    {
        return Iterator(Index::INVALID, *this);
    }

    return Iterator(index, *this);
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::ConstIterator
FixedPositionContainer<T, CAPACITY>::iteratorFromIndex(IndexType index) const noexcept
{
    if (index == Index::INVALID)
    {
        return ConstIterator(index, *this);
    }

    if (index > Index::LAST)
    {
        return ConstIterator(Index::INVALID, *this);
    }

    if (m_status[index] != SlotStatus::USED)
    {
        return ConstIterator(Index::INVALID, *this);
    }

    return ConstIterator(index, *this);
}

template <typename T, uint64_t CAPACITY>
T* FixedPositionContainer<T, CAPACITY>::elementAt(IndexType index) noexcept
{
    return std::launder(reinterpret_cast<T*>(&m_storage[static_cast<size_t>(index)]));
}

template <typename T, uint64_t CAPACITY>
const T* FixedPositionContainer<T, CAPACITY>::elementAt(IndexType index) const noexcept
{
    return std::launder(reinterpret_cast<const T*>(&m_storage[static_cast<size_t>(index)]));
}

template <typename T, uint64_t CAPACITY>
void FixedPositionContainer<T, CAPACITY>::destroyElement(IndexType index) noexcept
{
    IOX_ENFORCE(m_status[index] == SlotStatus::USED, "Attempting to destroy an empty slot!");
    std::destroy_at(elementAt(index));
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::IndexType
FixedPositionContainer<T, CAPACITY>::acquireSlot() noexcept
{
    IOX_ENFORCE(m_begin_free != Index::INVALID, "Container is full!");
    const IndexType slot = m_begin_free;
    m_begin_free = m_nextFree[slot];
    m_nextFree[slot] = Index::INVALID;
    m_status[slot] = SlotStatus::USED;
    return slot;
}

template <typename T, uint64_t CAPACITY>
void FixedPositionContainer<T, CAPACITY>::releaseSlot(IndexType index) noexcept
{
    m_status[index] = SlotStatus::FREE;
    m_nextUsed[index] = Index::INVALID;
    m_nextFree[index] = m_begin_free;
    m_begin_free = index;
}

template <typename T, uint64_t CAPACITY>
void FixedPositionContainer<T, CAPACITY>::addUsedSlot(IndexType index) noexcept
{
    if (m_begin_used == Index::INVALID || index < m_begin_used)
    {
        m_nextUsed[index] = m_begin_used;
        m_begin_used = index;
        return;
    }

    IndexType previous = m_begin_used;
    IndexType current = m_nextUsed[previous];
    while (current != Index::INVALID && current < index)
    {
        previous = current;
        current = m_nextUsed[current];
    }

    m_nextUsed[previous] = index;
    m_nextUsed[index] = current;
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::IndexType
FixedPositionContainer<T, CAPACITY>::removeUsedSlot(IndexType index) noexcept
{
    IOX_ENFORCE(m_begin_used != Index::INVALID, "Cannot remove from an empty container!");

    if (m_begin_used == index)
    {
        m_begin_used = m_nextUsed[index];
        m_nextUsed[index] = Index::INVALID;
        return m_begin_used;
    }

    IndexType previous = m_begin_used;
    while (previous != Index::INVALID && m_nextUsed[previous] != index)
    {
        previous = m_nextUsed[previous];
    }

    IOX_ENFORCE(previous != Index::INVALID, "Index not part of used list!");
    const IndexType next = m_nextUsed[index];
    m_nextUsed[previous] = next;
    m_nextUsed[index] = Index::INVALID;
    return next;
}

template <typename T, uint64_t CAPACITY>
typename FixedPositionContainer<T, CAPACITY>::IndexType
FixedPositionContainer<T, CAPACITY>::findIndexFromPointer(const T* ptr) const noexcept
{
    IndexType current = m_begin_used;
    while (current != Index::INVALID)
    {
        if (elementAt(current) == ptr)
        {
            return current;
        }
        current = m_nextUsed[current];
    }

    return Index::INVALID;
}

} // namespace ZeroCP

#endif // ZEROCP_FOUNDATION_VOCAB_FIXED_POSITION_CONTAINER_INL

