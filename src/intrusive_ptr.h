#ifndef SIMIT_INTRUSIVE_PTR_H
#define SIMIT_INTRUSIVE_PTR_H

namespace simit {
namespace util {

/// This class provides an intrusive pointer, which is a pointer that stores its
/// reference count in the managed class.  The managed class must therefore have
/// a reference count field and provide two functions 'aquire' and 'release'
/// to aquire and release a reference on itself.
///
/// For example:
/// struct X {
///   mutable long ref;
///   X() : ref(0) {}
///
///   void aquire(X *x) { ++x->ref; }
///   void release(X *x) { if (--x->ref ==0) delete x; }
/// };
template <class T>
class IntrusivePtr {
public:
  /// Allocate an undefined IntrusivePtr
  IntrusivePtr() : ptr(nullptr) {}

  /// Allocate an IntrusivePtr with an object
  IntrusivePtr(T *ptr) : ptr(ptr) {
    if (ptr) {
      aquire(ptr);
    }
  }

  /// Copy constructor
  IntrusivePtr(const IntrusivePtr &other) : ptr(other.ptr) {
    if (ptr) {
      aquire(ptr);
    }
  }

  /// Move constructor
  IntrusivePtr(IntrusivePtr &&other) : ptr(other.ptr) {
    other.ptr = nullptr;
  }

  /// Copy assignment operator
  IntrusivePtr& operator=(const IntrusivePtr &other) {
    ptr = other.ptr;
    if (ptr) {
      aquire(ptr);
    }
    return *this;
  }

  /// Copy assignment operator for managed object
  IntrusivePtr& operator=(T *ptr) {
    this->ptr = ptr;
    if (ptr) {
      aquire(ptr);
    }
  }

  /// Move assignment operator
  IntrusivePtr& operator=(IntrusivePtr &&other) {
    ptr = other.ptr;
    other.ptr = nullptr;
    return *this;
  }

  /// Destroy the intrusive ptr.
  virtual ~IntrusivePtr() {
    if (ptr) {
      release(ptr);
    }
  }

  /// Check whether the pointer is defined (ptr is not null).
  bool defined() const {return ptr != nullptr;}

  friend inline
  bool operator==(const IntrusivePtr<T> &p1, const IntrusivePtr<T> &p2) {
    return p1.ptr == p2.ptr;
  }

  friend inline
  bool operator!=(const IntrusivePtr<T> &p1, const IntrusivePtr<T> &p2) {
    return p1.ptr != p2.ptr;
  }

protected:
  T *ptr;
};

}} // namespace simit::util

#endif