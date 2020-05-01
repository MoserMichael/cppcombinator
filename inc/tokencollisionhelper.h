#pragma once

#include "parse_base.h"
#include <unordered_map>
#include <set>
#include <typeinfo>

namespace pparse {

using TokenHash = unsigned long;

//
// The collision checker is a helper object.
// it is used to check that variables that have the string value of a token are not accepted as a variable;
//
struct TokenCollisionChecker {

    static constexpr TokenHash TokenHashInitValue = 5381;
    using Map_type =  std::unordered_multimap< TokenHash, std::string >; 

    struct type_info_compare {
        bool operator() (const std::type_info *a, const std::type_info *b) const {
            return a->before(*b);
        }
    };
    
    using Type_info_set_type = std::set<const std::type_info *, type_info_compare>;

    // pick Dan Bernstein hash from here  https://stackoverflow.com/questions/7666509/hash-function-for-string 
    static inline TokenHash calculate_hash_val(TokenHash hash, Char_t ch) {
        return (hash << 5) + hash + ch;
    }
    
    static inline TokenHash calculate_hash(const Char_t *token, uint32_t token_len) {
        TokenHash hash = TokenHashInitValue;

        for(uint32_t pos=0; pos < token_len; ++pos) {
            hash = calculate_hash_val( hash, token[pos]);
        }
        return hash;
    }


    bool insert(const Char_t * token, uint32_t token_len) {

        printf("Insert %s(%d)\n", token,token_len);

        TokenHash hash = calculate_hash( token, token_len );
    
        if (has_token_imp(token, token_len, hash)) {
            printf("already exists\n");
            return true;
        }

        //mapHashToToken.emplace( hash, std::string( token, token_len ) );
        mapHashToToken.insert( Map_type::value_type( hash, std::string( token, token_len ) ) );
        printf("inserted\n");
        return true;
    }

    bool has_token(const Char_t *token, uint32_t token_len) {
       TokenHash hash = calculate_hash( token, token_len );
       return has_token_imp( token, token_len, hash );
    } 


    bool insert_type_info(const std::type_info *info) {
        auto ret = tinfo_set.insert(info);
        return ret.second;
    }

    bool remove_type_info(const std::type_info *info) {
            return tinfo_set.erase(info) > 0;
    }

private:

    inline bool has_token_imp(const Char_t *token, uint32_t token_len, TokenHash hash) {
        auto pos = mapHashToToken.find( hash );
       if (pos == mapHashToToken.end()) {
           return false;
       }

       if (pos->second.size() != token_len) {
            return false;
       }

       if (strncmp(pos->second.c_str(), token, token_len) == 0) {
            return true;
       }
       return false;
    }

    Map_type mapHashToToken;
    Type_info_set_type tinfo_set;
};
    

} // namespace pparse


