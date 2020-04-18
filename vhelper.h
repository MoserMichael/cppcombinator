#ifdef __PARSER_TRACE__

namespace pparse {

const int PREFIX_NSPACE_LEN = 8;
const char *CHAR_SYM="(char)";
const int CHAR_SYM_LEN=6;


static inline size_t __thread nesting_ = 0;

template<typename Type>
struct VisualizeTrace {

		using ThisClass = VisualizeTrace<Type>; 

		static std::string  trace_start_parsing(Text_position pos) {
			std::string short_name = make_short_name();
			printf("(%03ld)%sstart parsing: %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_);
			nesting_ += 1;
			return short_name;
		}

		static void end_parsing(std::string &short_name, bool success,Text_position pos) {
			nesting_ -= 1;
			printf("(%03ld%send parsing: %s %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), success ? "SUCCESS" : "FAIL", pos.line_, pos.column_, pos.buffer_pos_ );
		}

		static void end_parsing_choice(std::string &short_name, bool success,Text_position pos, size_t index) {
			nesting_ -= 1;

			if (success) {
				printf("(%03ld)%send parsing: %s SUCCESS choice-index: %ld at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), index , pos.line_, pos.column_, pos.buffer_pos_ );
			} else {

				printf("(%03ld)%send parsing: %s FAIL at (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_ );
			}
		}


		static std::string  trace_start_parsing_token(Text_position pos) {
			std::string short_name = make_token_name();
			
			printf("(%03ld)%sstart parsing: %s at: (%d:%d offset: %ld)\n", nesting_, std::string( nesting_, ' ').c_str(), short_name.c_str(), pos.line_, pos.column_, pos.buffer_pos_);
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

					size_t nposc = component.rfind(':');
					if (nposc != std::string::npos) {
						nposc += 1;
						rname += " " + component.substr(nposc, component.size() - nposc);
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

