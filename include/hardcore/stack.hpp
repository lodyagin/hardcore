// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
///////////////////////////////////////////////////////////

/**
 * @file
 * Access to stack frames.
 *
 * @author Sergei Lodyagin
 */

#ifndef HARDCORE_STACK_HPP
#define HARDCORE_STACK_HPP

#include <iostream>
#include <cstddef>
#include <cassert>
#include <iterator>
#include <cassert>
#include <sys/resource.h>

extern void* __libc_stack_end;

namespace hc {

class stack;

namespace stack_ {

/**
   Links one frame to another, usually it is constructed
   as

   call ...
   pushq  %rbp
   movq   %rsp, %rbp
*/
struct link
{
  using fun_t = void();
  using fun_ptr_t = fun_t*;

  link* up;
  fun_ptr_t ret;
};

struct type
{
  using fp_type = const link*;
  using ip_type = link::fun_ptr_t;

  fp_type fp;
  ip_type ip;

  operator fp_type() const
  {
    return fp;
  }

  operator ip_type() const
  {
    return ip;
  }
};

template<class Value>
class iterator : protected type
{
  friend class hc::stack;

public:
  using iterator_category = std::forward_iterator_tag;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = Value;
  using reference = Value;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  constexpr iterator() : type{nullptr, nullptr} {}

  bool operator==(const iterator& o) const
  {
    return fp == o.fp && ip == o.ip;
  }

  bool operator!=(const iterator& o) const
  {
    return !operator==(o);
  }

  inline iterator& operator++();
 
  iterator operator++(int)
  {
    iterator copy = *this;
    operator++();
    return copy;
  }

  iterator& operator+=(size_t k)
  {
    std::advance(*this, k);
    return *this;
  }

  iterator operator+(size_t k)
  {
    iterator res = *this;
    return res += k;
  }

  reference operator*() const
  {
    return (reference) *this;
  }

  /*const_pointer operator->() const
  {
    return this;
  }*/

  bool is_end() const
  {
    return fp == nullptr;
  }

#if 0
  iterator(const link* fp_, pointer ip_)
    __attribute__ ((always_inline))
    : type{fp_, ip_}
  {}
#else
  iterator(const type& t) //__attribute__ ((always_inline))
    : type(t)
  {}
#endif

};

static ptrdiff_t frame_offset(type::fp_type fp)
{
    return (char*)fp - (char*)__libc_stack_end;
}

} // stack_

//! The current stack. You can pass this object to
//! underlying functions but never can return it.
//! @tparam limit browsable stack size
//template<std::size_t limit = 35> TODO
class stack : protected stack_::type
{
public:
  using difference_type = std::ptrdiff_t;
  using value_type = stack_::type;
  using iterator = stack_::iterator<value_type>;
  using reference = iterator::reference;
  using size_type = typename iterator::size_type;

  using typename stack_::type::fp_type;
  using typename stack_::type::ip_type;

  using ip_iterator = stack_::iterator<ip_type>;

protected:
  //! The caller's instruction pointer
  static ip_type callers_ip() 
    __attribute__ ((always_inline))
  {
    return (ip_type) __builtin_return_address(0);
  }

public:
  static size_t max_size()
  {
      static size_t stack_size = 0;

      if (stack_size == 0)
      {
          struct rlimit rl;
          if (getrlimit(RLIMIT_STACK, &rl) != 0)
          {
              assert(false);
              exit(1); // must never happen
          }

          stack_size = rl.rlim_cur;
      }
      return stack_size;
  }

#if 1
  stack() noexcept : type{nullptr, nullptr}
  {
  }
#else
  // problem with optimization, try to compile 
  // 7c4121f02ed3e2bd with -O3 (gcc 4.9.1)
  stack() __attribute__ ((always_inline))
    : type{
        (fp_type) __builtin_frame_address(0),
        (ip_type) callers_ip()
      }
  {}
#endif

  stack(const stack_::type& o) : type(o) {}

  //! The current frame (top of the stack)
  static reference top() __attribute__ ((noinline))
  {
    return type{
      (fp_type) __builtin_frame_address(1),
      (ip_type) callers_ip()
    };
  }

  //! returns iterator to top element
  static iterator begin() __attribute__ ((always_inline))
  {
    return top();
  }

  static ip_iterator ip_begin() __attribute__ ((always_inline))
  {
    return top();
  }

  iterator end()
  {
    return iterator();
  }

  ip_iterator ip_end()
  {
    return ip_iterator();
  }

  static bool is_valid_frame(stack_::type::fp_type frame)
  {
    const ptrdiff_t fo = hc::stack_::frame_offset(frame);
    return (   
        __builtin_expect(fo <= 0, 1)
     && __builtin_expect(fo >= -(ptrdiff_t) hc::stack::max_size(), 1));
  }

  //! Marker type for output ips only
  struct ips
  {
    ips()__attribute__((__always_inline__))
      : bg(stack().begin()) {}

    iterator bg;
  };
};

std::ostream&
operator<<(std::ostream& out, const stack::ips& ips);

namespace stack_ {

template<class Value>
iterator<Value>& iterator<Value>::operator++()
{
  if (__builtin_expect(fp == nullptr, 0)) {
    ip = nullptr;
  }
  else {
    if (// broken stack
           !stack::is_valid_frame(fp)
        || !stack::is_valid_frame(fp->up)
        || __builtin_expect(fp->up <= fp, 0)
        )
    {
      // end()
      ip = nullptr; 
      fp = nullptr;
    }
    else {
      ip = fp->ret;
      fp = fp->up;
    }
  }
  return *this;
}

} // stack_

} // hc

#endif
