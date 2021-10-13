#pragma once
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <utility>
#include <algorithm>

template <typename T>
struct circular_buffer {
  template <typename U>
  struct basic_iterator;

  using iterator = basic_iterator<T>;
  using const_iterator = basic_iterator<T const>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  circular_buffer() noexcept                   //O(1)
    : arr(nullptr)
    , sz(0)
    , cap(0)
    , head(0)
  {};

  circular_buffer(circular_buffer const& other)    // O(n), strong
    : circular_buffer()
  {
    reserve(other.size());
    for (auto it = other.begin(); it != other.end(); ++it) {
      push_back(*it);
    }
  }
  ~circular_buffer() {                               // O(n)
    clear();
    operator delete(arr);
  }
  circular_buffer& operator=(circular_buffer other){  // O(n), strong
    if (this == &other) {
      return *this;
    }
    swap(other);
    other.clear();
    return *this;
  }

  size_t size() const noexcept {                     // O(1)
    return sz;
  }
  T& operator[](size_t index) noexcept {             // O(1)
    return arr[(head + index) % cap];
  }
  T const& operator[](size_t index) const noexcept {  // O(1)
    return const_cast<T const&>(arr[(head + index) % cap]);
  }

  bool empty() const noexcept {                     // O(1), nothrow
    return sz == 0;
  }
  void clear() noexcept {                          // O(n), nothrow
    for (auto it = begin(); it != end(); ++it) {
      it->~T();
    }
    sz = 0;
    head = 0;
  }

  void push_back(T const& val) { // O(1), strong
    T val_copy(val);
    ensure_cap();
    if (empty()) {
      new (arr) T(val_copy);
    } else {
      new (arr + ((tail() + 1) % cap)) T(val_copy);
    }
    ++sz;
  }
  void pop_back() noexcept { // O(1)
    arr[tail()].~T();
    --sz;
  }
  T& back() noexcept {            // O(1)
    return arr[tail()];
  }
  T const& back() const noexcept { // O(1)
    return arr[tail()];
  }

  void push_front(T const& val) { // O(1), strong
    T val_copy(val);
    ensure_cap();
    if (!empty()) {
      head = (head + cap - 1) % cap;
    }
    new (arr + head) T(val_copy);
    ++sz;
  }
  void pop_front() noexcept { // O(1)
    arr[head].~T();
    head = (head + 1) % cap;
    --sz;
  }
  T& front() noexcept {            // O(1)
    return arr[head];
  }
  T const& front() const noexcept { // O(1)
    return arr[head];
  }

  void reserve(size_t desired_capacity) { // O(n), strong
    increase_cap(desired_capacity);
  }
  size_t capacity() const noexcept {      // O(1)
    return cap;
  }

  iterator begin() noexcept {             // O(1)
    return iterator(node(this, 0));
  }
  const_iterator begin() const noexcept { // O(1)
    return const_iterator(node(this, 0));
  }
  iterator end() noexcept {
    return iterator(node(this, sz));
  }
  const_iterator end() const noexcept { // O(1)
    return const_iterator(node(this, sz));
  }

  reverse_iterator rbegin() noexcept { // O(1)
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const noexcept { // O(1)
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept { // O(1)
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const noexcept { // O(1)
    return const_reverse_iterator(begin());
  }

  iterator insert(const_iterator pos, T const& val) {     // O(n), basic
    if (pos - begin() < end() - pos) {
      push_front(val);
      auto cur = begin();
      while (cur < pos) {
        auto nxt = cur + 1;
        std::swap(*cur, *nxt);
        ++cur;
      }
      return cur;
    } else {
      push_back(val);
      auto cur = end() - 1;
      while (cur > pos) {
        auto prv = cur - 1;
        std::swap(*cur, *prv);
        --cur;
      }
      return cur;
    }
  }

  iterator erase(const_iterator pos) {                         // O(n), basic
    size_t index = pos.get_arr_index();
    if (pos - begin() < end() - pos) {
      iterator cur(node(this, index));
      while (cur > begin()) {
        auto prv = cur - 1;
        std::swap(*prv, *cur);
        --cur;
      }
      pop_front();
    } else {
      iterator cur(node(this, index));
      while (cur < end() - 1) {
        auto nxt = cur + 1;
        std::swap(*nxt, *cur);
        ++cur;
      }
      pop_back();
    }
    return iterator(node(this, index));
  }

  iterator erase(const_iterator first, const_iterator last) {  // O(n), basic
    size_t first_index = first.get_arr_index();
    size_t last_index = last.get_arr_index();
    if (first - begin() < end() - last) {
      if (first > begin()) {
        iterator end_1 = iterator(node(this, first_index - 1));
        iterator end_2 = iterator(node(this, last_index - 1));
        while (end_1 > begin()) {
          std::swap(*end_1, *end_2);
          --end_1, --end_2;
        }
      }
      for (size_t i = 0; i < last_index - first_index; ++i) {
        pop_front();
      }
    } else {
      if (last != end()) {
        iterator begin_1 = iterator(node(this, last_index));
        iterator begin_2 = iterator(node(this, first_index));
        while (begin_1 < end()) {
          std::swap(*begin_1, *begin_2);
          ++begin_1, ++begin_2;
        }
      }
      for (size_t i = 0; i < last_index - first_index; ++i) {
        pop_back();
      }
    }
    return iterator(node(this, first_index));
  }

  void swap(circular_buffer& other) noexcept {               // O(1)
    std::swap(head, other.head);
    std::swap(sz, other.sz);
    std::swap(cap, other.cap);
    std::swap(arr, other.arr);
  }

private:
  int head;
  size_t sz;
  size_t cap;
  T* arr;

  int tail() const {
    if (sz == 0) {
      return head;
    }
    return (head + sz + cap - 1) % cap;
  }

  struct node {
    circular_buffer* buf;
    size_t arr_index;

    node() = default;

    node(circular_buffer const* _buf, size_t const _arr_index)
      : buf(const_cast<circular_buffer*>(_buf))
      , arr_index(_arr_index)
    {}

    node(node const& other)
      : buf(other.buf)
      , arr_index(other.arr_index)
    {}
  };

  void copyToArray(T* dest) {
    if (head <= tail()) {
      for (size_t i = head; i < head + sz; ++i) {
        try {
          new (dest + (i - head)) T(arr[i]);
        } catch(...) {
          if (i > head) {
            delete_array(dest, 0, i - head - 1);
            operator delete(dest);
          }
          throw;
        }
      }
    } else {
      int head_len = cap - head;
      for (size_t i = head; i < cap; ++i) {
        try {
          new (dest + (i - head)) T(arr[i]);
        } catch(...) {
          if (i > head) {
            delete_array(dest, 0, i - head - 1);
            operator delete(dest);
          }
          throw;
        }
      }
      for (size_t i = 0; i <= tail(); ++i) {
        try {
          new (dest + (i + head_len)) T(arr[i]);
        } catch(...) {
          if (i > 0) {
            delete_array(dest, head_len, head_len + i - 1);
            operator delete(dest);
          }
        }
      }
    }
  }

  void increase_cap(size_t new_cap) {
    if (new_cap >= cap) {
      T* new_arr = static_cast<T*>(operator new((new_cap + 1) * sizeof(T)));
      size_t old_sz = sz;
      copyToArray(new_arr);
      clear();
      if (arr != nullptr) {
        operator delete(arr);
      }
      cap = new_cap + 1;
      sz = old_sz;
      arr = new_arr;
    }
  }

  void ensure_cap() {
    if (sz + 1 >= cap) {
      if (cap == 0) {
        increase_cap(2);
      } else {
        increase_cap(2 * (cap - 1) + 1);
      }
    }
  }

  void delete_array(T* array, size_t l, size_t r) {
    for (size_t i = r + 1; i >= l + 1; --i) {
      array[i - 1].~T();
    }
  }

  int get_arr_pos(size_t index) {
    return (head + index) % cap;
  }
};

template <typename T>
template <typename U>
struct circular_buffer<T>::basic_iterator
{
  using iterator_category = std::random_access_iterator_tag;
  using value_type = U;
  using difference_type = ptrdiff_t;
  using pointer = U*;
  using reference = U&;

  basic_iterator() = default;
  basic_iterator(basic_iterator const&) = default;

  template <typename V, typename = std::enable_if_t<std::is_const_v<U> && !std::is_const_v<V>>>
  basic_iterator(basic_iterator<V> const& other)
    : nd(other.nd)
  {}

  U& operator*() const {
    return nd.buf->arr[nd.buf->get_arr_pos(nd.arr_index)];
  }
  U* operator->() const {
    return &nd.buf->arr[nd.buf->get_arr_pos(nd.arr_index)];
  }

  basic_iterator& operator++() & {
    return *this += 1;
  }
  basic_iterator operator++(int) &{
    basic_iterator copy(*this);
    ++*this;
    return copy;
  }

  basic_iterator& operator--() & {
    return *this -= 1;
  }
  basic_iterator operator--(int) & {
    basic_iterator copy(*this);
    --*this;
    return copy;
  }

  friend bool operator==(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.buf == b.nd.buf && a.nd.arr_index == b.nd.arr_index;
  }

  friend bool operator!=(basic_iterator const& a, basic_iterator const& b) {
    return !(a == b);
  }

  friend bool operator<(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.buf == b.nd.buf && a.nd.arr_index < b.nd.arr_index;
  }

  friend bool operator>(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.buf == b.nd.buf && a.nd.arr_index > b.nd.arr_index;
  }

  friend bool operator<=(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.buf == b.nd.buf && a.nd.arr_index <= b.nd.arr_index;
  }

  friend bool operator>=(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.buf == b.nd.buf && a.nd.arr_index >= b.nd.arr_index;
  }

  reference operator[](size_t index) {  // O(1)
    return nd.buf->arr[nd.buf->get_arr_pos(nd.arr_index + index)];
  }

  basic_iterator& operator+=(difference_type k) {
    nd.arr_index += k;
    return *this;
  }

  basic_iterator& operator-=(difference_type k) {
    nd.arr_index -= k;
    return *this;
  }

  friend basic_iterator operator+(basic_iterator it, difference_type k) {
    it += k;
    return it;
  }

  friend basic_iterator operator-(basic_iterator it, difference_type k) {
    it -= k;
    return it;
  }

  friend basic_iterator operator+(difference_type k, basic_iterator it) {
    it += k;
    return it;
  }

  friend difference_type operator-(basic_iterator const& a, basic_iterator const& b) {
    return a.nd.arr_index - b.nd.arr_index;
  }

private:
  explicit basic_iterator(node const& _nd)
    : nd(_nd)
  {}

  node nd;

  size_t get_arr_index() {
    return nd.arr_index;
  }

  friend struct circular_buffer<T>;
};