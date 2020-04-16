#pragma once

#ifdef __PARSER_TRACE__

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

const int PREFIX_NSPACE_LEN = 8;
const char *CHAR_SYM="(char)";
const int CHAR_SYM_LEN=6;


static inline size_t __thread nesting_ = 0;

template<typename Type>
struct VisualizeTrace {

		using ThisClass = VisualizeTrace<Type>; 

		static std::string  trace_start_parsing(Text_position pos) {
			std::string short_name = make_short_name();
			printf("(%03d)%sstart parsing: %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_);
			nesting_ += 1;
			return short_name;
		}

		static void end_parsing(std::string &short_name, bool success,Text_position pos) {
			nesting_ -= 1;
			printf("(%03d)%send parsing: %s %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), success ? "SUCCESS" : "FAIL", pos.line_, pos.column_, pos.buffer_pos_ );
		}

		static void end_parsing_choice(std::string &short_name, bool success,Text_position pos, size_t index) {
			nesting_ -= 1;

			if (success) {
				printf("(%03d)%send parsing: %s SUCCESS choice-index: %ld at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), index , pos.line_, pos.column_, pos.buffer_pos_ );
			} else {

				printf("(%03d)%send parsing: %s FAIL at (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_ );
			}
		}


		static std::string  trace_start_parsing_token(Text_position pos) {
			std::string short_name = make_token_name();
			
			printf("(%03d)%sstart parsing: %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_);
			nesting_ += 1;
			
			return short_name;
		}
	


private:

		static std::string make_token_name() {

			std::string rval("token: ");	

			size_t pos = 0;
			std::string cname = demangle<Type>();
			while(pos < cname.size()) {
				size_t next_pos = cname.find( CHAR_SYM, pos);
				if (next_pos == std::string::npos) {
					break;
				}
				next_pos += CHAR_SYM_LEN;

				size_t eof_pos = cname.find( ' ', next_pos);
				if (eof_pos == std::string::npos) {
					eof_pos = cname.size();
				}
				
				std::string tokval = cname.substr(next_pos, eof_pos - next_pos);

				size_t nchar = atoi(tokval.c_str());

				rval += (Char_t) nchar; 

				pos = eof_pos;
			}
			return rval;
		}


		using Component_t = std::pair<size_t, size_t>;

		static Component_t find_component_name(std::string dname, size_t start_pos) 
		{
		 	size_t next_pos = start_pos;

			if ( next_pos >= dname.size() ) {
				return Component_t(std::string::npos,0);
			}

			int template_nesting = 0, max_template_nesting = 0;

			for(next_pos += 1;next_pos < dname.size(); ++next_pos)  {

				if (dname[next_pos] == '<') {
					template_nesting += 1;
					max_template_nesting += 1;
				} else if (dname[next_pos] == '>') {
					template_nesting -= 1;
					if (template_nesting == 0) {
						break;
					}
				} else if (dname[next_pos] == ',' && template_nesting == 0) {
					break;
				}
			}

			return Component_t(next_pos+1, max_template_nesting);

		}

		static std::string make_short_name(std::string dname) {
			if (dname.substr(0,PREFIX_NSPACE_LEN) != "pparse::") {
				return "";
			}

			size_t npos = dname.find( '<', PREFIX_NSPACE_LEN);
			if (npos == std::string::npos) {
				return "";
			}
			npos = dname.find( ' ', npos);
			if (npos == std::string::npos) {
				return "";
			}

			std::string rname = dname.substr(PREFIX_NSPACE_LEN, npos - PREFIX_NSPACE_LEN );

			Component_t ret;

			while((ret = find_component_name(dname, npos)), ret.first != std::string::npos) {

				std::string component = dname.substr(npos + 1, ret.first - npos - 1);

				if (ret.second == 0) {

					size_t npos = component.rfind(':');
					if (npos != std::string::npos) {
						npos += 1;
						rname += " " + component.substr(npos, component.size() - npos);
					} else {
						rname += " " + component;
					}
				} else {
					auto pos_start = component.find('<');
					if (pos_start != std::string::npos) {
						pos_start = component.find(' ', pos_start+1);
						if (pos_start != std::string::npos) {
							pos_start -= 1;
						} else {
							pos_start = component.size()-1;
						}
					}

					rname += " " + component.substr(0,pos_start) + "> ";

//					auto pos_tok = component.find("::");
//					if (pos_start!= std::string::npos) {
//						if ((pos_tok+2) < pos_start && pos_tok  != std::string::npos) {
//							rname += " " + component.substr(pos_tok + 2, pos_start - pos_tok - 2) + "> ";
//						} else {
//							rname += " " + component.substr(0,pos_start) + "> ";
//						}
//					}
				}
				npos = dname.find(' ', ret.first);
				if (npos == std::string::npos) {
					break;
				}
			}

			return rname;

		}

		static std::string make_short_name() {
			std::string cname = demangle<Type>();
		
			std::string shname = make_short_name(cname);
			if (shname == "") {
				return cname;
			}
			return shname;
		}
	
	};	


} // eof namespace pparse

// eof __PARSER_TRACE__ 
#endif


