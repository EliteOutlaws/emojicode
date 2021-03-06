//
//  Runtime.h
//  Emojicode
//
//  Created by Theo Weidmann on 09/09/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef Runtime_h
#define Runtime_h

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <new>

namespace runtime {
namespace internal {
struct ControlBlock;
ControlBlock* newControlBlock();
}
}

extern "C" int8_t* ejcAlloc(int64_t size);
extern "C" [[noreturn]] void ejcPanic(const char *message);

namespace runtime {

using Integer = int64_t;
using Symbol = uint32_t;
using Boolean = int8_t;
using Byte = int8_t;
using Real = double;
using Enum = int64_t;

template <typename Subclass>
struct ClassInfoFor;

namespace util {

template <class T, std::size_t = sizeof(T)>
std::true_type is_complete_impl(T *);
std::false_type is_complete_impl(...);
template <class T>
using is_complete = decltype(is_complete_impl(std::declval<T*>()));

}  // namespace util

struct ClassInfo {
    ClassInfo *superclass;
    void **dispatchTable;

    template <typename Return, typename ObjectType, typename ...Args>
    Return dispatch(size_t virtualTableIndex, ObjectType *object, Args... args) const {
        return reinterpret_cast<Return(*)(ObjectType*, Args...)>(dispatchTable[virtualTableIndex])(object, args...);
    }
};

template <typename T>
class MemoryPointer {
    template <typename TA>
    friend inline MemoryPointer<TA> allocate(int64_t n);
public:
    MemoryPointer() {}
    T* get() const {
        return reinterpret_cast<T*>(pointer_ + sizeof(runtime::internal::ControlBlock *));
    }

    T& operator[](size_t index) const {
        return get()[index];
    }
private:
    explicit MemoryPointer(int8_t *pointer) : pointer_(pointer) {}
    int8_t* pointer_ = nullptr;
};

template <typename T>
inline MemoryPointer<T> allocate(int64_t n = 1) {
    return MemoryPointer<T>(ejcAlloc(sizeof(T) * n + sizeof(runtime::internal::ControlBlock *)));
}

template <typename Subclass>
class Object {
public:
    template <typename ...Args>
    static Subclass* init(Args ...args) {
        static_assert(util::is_complete<ClassInfoFor<Subclass>>::value,
                      "Provide class info for this class with SET_INFO_FOR.");
        return new(malloc(sizeof(Subclass))) Subclass(args...);
    }

    internal::ControlBlock* controlBlock() const { return block_; }
    const ClassInfo* classInfo() const { return classInfo_; }
protected:
    Object() : block_(internal::newControlBlock()), classInfo_(ClassInfoFor<Subclass>::value) {}
private:
    internal::ControlBlock *block_;
    const ClassInfo *classInfo_;
};

#define CLASS_INFO_NAME(package, emojitypename) package ## _class_info_ ## emojitypename
#define SET_INFO_FOR(type, package, emojitypename) extern "C" const runtime::ClassInfo CLASS_INFO_NAME(package, emojitypename); \
template<> struct runtime::ClassInfoFor<type> { constexpr static const runtime::ClassInfo *const value = &CLASS_INFO_NAME(package, emojitypename); };

struct NoValue_t {};
constexpr NoValue_t NoValue {};

template <typename Type>
class SimpleOptional {
public:
    SimpleOptional(NoValue_t) : value_(0) {}
    SimpleOptional(Type content) : value_(1), content_(content) {}

    bool operator==(NoValue_t) const {
        return !value_;
    }

    Type& operator*() {
        return content_;
    }

    const Type& operator*() const {
        return content_;
    }
private:
    Boolean value_;
    Type content_;
};

struct MakeError_t {};
constexpr MakeError_t MakeError {};

template <typename Type>
class SimpleError {
public:
    SimpleError(MakeError_t, Enum errorEnumValue) : errorValue_(errorEnumValue) {}
    SimpleError(Type content) : errorValue_(-1), content_(content) {}
private:
    Enum errorValue_;
    Type content_;
};

template <typename Return, typename ...Args>
class Callable {
public:
    Return operator ()(Args... args) const {
        return function_(catpures_, args...);
    }
private:
    Return (*function_)(void*, Args...);
    void *catpures_;
};

}

#endif /* Runtime_h */
