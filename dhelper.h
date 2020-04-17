#pragma once

#if defined(__PARSER_TRACE__) || defined(__PARSER_ANALYSE__)

#include <cxxabi.h>
#include <memory>
#include <iostream>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <cassert>
#include <stdexcept>
#include <string.h>


namespace pparse {

//
// This trick is copied from: https://stackoverflow.com/questions/37764459/getting-class-name-as-a-string-in-static-method-in-c . Thanks! 
// But now the tracing feature depends on the presence of RTTI. Don't know any better than this.
//

struct demangled_string
{
    using ptr_type = std::unique_ptr<char, void(*)(void*)>;
    demangled_string(ptr_type&& ptr) noexcept;
    const char* c_str() const;
    operator std::string() const;

    std::ostream& write(std::ostream& os) const;
private:
    ptr_type _ptr;
};

inline std::ostream& operator<<(std::ostream& os, const demangled_string& str)
{
    return str.write(os);
}

inline std::string operator+ (std::string l, const demangled_string& r) {
    return l + r.c_str();
}

inline std::string operator+(const demangled_string& l, const std::string& r)
{
    return std::string(l) + r;
}

demangled_string demangle(const char* name);
demangled_string demangle(const std::type_info& type);
demangled_string demangle(std::type_index type);

template<class T>
demangled_string demangle(T* p) {
    return demangle(typeid(*p));
}

template<class T>
demangled_string demangle()
{
    return demangle(typeid(T));
}


// implementation

demangled_string::demangled_string(ptr_type&& ptr) noexcept
: _ptr(std::move(ptr))
{}

std::ostream& demangled_string::write(std::ostream& os) const
{
    if (_ptr) {
        return os << _ptr.get();
    }
    else {
        return os << "{nullptr}";
    }
}

const char* demangled_string::c_str() const
{
    if (!_ptr)
    {
        throw std::logic_error("demangled_string - zombie object");
    }
    else {
        return _ptr.get();
    }
}

demangled_string::operator std::string() const {
    return std::string(c_str());
}


demangled_string demangle(const char* name)
{
    using namespace std::string_literals;

    int status = -4;

    demangled_string::ptr_type ptr {
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free
    };

    if (status == 0) return { std::move(ptr) };

    switch(status)
    {
        case -1: throw std::bad_alloc();
        case -2: {
            std::string msg = "invalid mangled name~";
            msg += name;
            auto p = (char*)std::malloc(msg.length() + 1);
            strcpy(p, msg.c_str());
            return demangled_string::ptr_type { p, std::free };
        }
        case -3:
            assert(!"invalid argument sent to __cxa_demangle");
            throw std::logic_error("invalid argument sent to __cxa_demangle");
        default:
            assert(!"PANIC! unexpected return value");
            throw std::logic_error("PANIC! unexpected return value");
    }
}

demangled_string demangle(const std::type_info& type)
{
    return demangle(type.name());
}

demangled_string demangle(std::type_index type)
{
    return demangle(type.name());
}

std::string method(const demangled_string& cls, const char* method)
{
    return std::string(cls) + "::" + method;
}
#endif

}
