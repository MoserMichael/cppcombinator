#ifdef __PARSER_ANALYSE__
#include <list>
#include <typeinfo>

#include "dhelper.h"

namespace pparse {

struct CycleDetectorHelper {

	struct Frame {
		const std::type_info *info_;
		int subterm_;
		std::string typename_;
	};

	using FrameList = std::list<Frame>;

	bool push_and_check(std::ostream &out, const std::type_info &typid, int subterm ) {
			
			bool no_cycles = true;

			for(auto pos=stack_.rbegin(); pos != stack_.rend(); ++pos) { 
					Frame &frame = *pos;
					if (typid == *frame.info_) { 
							// cycle detected.
							out << "cycle detected: \n";
							for(auto it = pos.base(); it != stack_.end(); ++it ) {
								out << "\t" << demangle(*it->info_);
								if (it->subterm_ != -1) {
									out << " term: " << it->subterm_;
								}
								out << "\n";
							}
							out << demangle(typid);
							out << "\n";
							no_cycles = false;
						}
			}
			stack_.push_back( Frame{ &typid, subterm } );
			return no_cycles;
	}

	void pop() {
			stack_.pop_back(); 
	}

	FrameList stack_;
};

template<typename Info>
static const std::type_info &get_tinfo(Info *ptr)  {
	return typeid(ptr);
}


template<typename Grammar> 
struct ParserChecker {
	static bool check(std::ostream &stream) {
		CycleDetectorHelper helper;
		
		helper.push_and_check(stream, get_tinfo((Grammar *) nullptr), -1);

		bool ret = Grammar::verify_no_cycles((void *) nullptr, helper, stream);

		helper.pop();

		return !ret;
	}
};

} //namespace

#endif

