#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <utility>
#include <memory>
#include <vector>
#include <errno.h>
#include <stdlib.h>
#include "parse_base.h"

namespace pparse {

struct Text_position {
    Text_position() : line_(1), column_(0), buffer_pos_((FilePos_t) 0) { }

    void next_char(Char_t ch) {
        if (ch == '\n') {
            ++line_;
            column_ = 0;
        } else {
            ++column_;
        }
        ++buffer_pos_;
    }

    int line_;
    int column_;
    FilePos_t buffer_pos_;
};


// head_ == tail_ : buffer is empty
// (tail+1) % size_ == head_ : buffer is full
// cursor_ in range [ head_, (head_+1) % size_, ....  tail_ ]
//
// tail_ - doesn't hold any data. the buffer can therefore hold size() - 1 bytes.`

class Text_ringbuffer {
public:

    bool init(uint32_t size) {
        buf_ = new Char_t[ size ];
        size_ = size;
        head_ = tail_ = cursor_ = 0;
        return buf_ != nullptr;
    }

	void close() {
		delete [] buf_;
		buf_ = nullptr;
        size_ = head_ = tail_ = cursor_ = 0;
 
	}

    ~Text_ringbuffer() {
        delete [] buf_;
    }

	bool resize(uint32_t must_have = 0) {
		uint32_t newsize;

		do {
				if (size_ <= ON_RESIZE_DOUBLE_BUFFER_TILL) {
					newsize = 2 * size_;
				} else {
					newsize = size_ + ON_RESIZE_DOUBLE_BUFFER_TILL;
				}
		} while( newsize < must_have );

		Char_t *newbuf = new Char_t[ newsize ];
		if (newbuf == nullptr) {
			return false;
		}

		memcpy(newbuf, buf_, size_);
		delete [] buf_;
		buf_ = newbuf;
		size_ = newsize;
		
		return true;
	}

    bool empty() const {
        return head_ == tail_;
    }

    bool full() const {
        return (tail_+1) % size_ == head_;
    }

    bool eof_cursor() const {
        return tail_ == cursor_;
    }

    bool inc_head() {
        if (!empty()) {
            head_ = (head_ + 1) % size_;
            return true;
        }
        return false;
    }

    bool inc_tail() {
        if (!full()) {
            tail_ = (tail_ + 1) % size_;
            return true;
        }
        return false;
    }

    bool inc_tail(uint32_t inc_by) {
        if (inc_by > free()) {
            return false;
        }

        tail_ = (tail_ + inc_by) % size_;
        return true;
    }

    uint32_t free() const {
        return size_ - size() - 1;
    }

	// number of text bytes in buffer
    uint32_t size() const { 
        if (head_ <= tail_) {
            return tail_ - head_;
        }
        return size_-head_ + tail_;
    }


    bool inc_cursor() {
        if (!is_valid_pos(cursor_)) {
            return false;
        }
        cursor_ = (cursor_ + 1) % size_;
        return true;
    }

    void set_cursor(FilePos_t  cursor_pos) {
        cursor_ = cursor_pos % size_;
    }

    void set_cursor_and_head(FilePos_t  cursor_pos) {
        head_ = cursor_ = cursor_pos % size_;
    }


    bool is_valid_pos(uint32_t cursor_pos) {
        auto next_tail = (tail_ + 1) % size_;
        if (head_ < next_tail) {
            if (cursor_pos < head_ || cursor_pos > next_tail) {
                return false;
            }
        } else {
            if  (cursor_pos > next_tail && cursor_pos < head_) {
                return false;
            }
        }
        return true;
    }
    Char_t cursor_char() const { 
        return buf_[cursor_];
    }

    uint32_t size_;
    Char_t *buf_;
    uint32_t head_,tail_, cursor_;
};

class Text_stream {
public:
    static const uint32_t Default_buf_size = 4096;

    using Next_char_value = std::pair<bool, Char_t>;

    Text_stream(bool resize_if_full = true) : fd_(-1), error_(0), pos_at_head_(0), resize_if_full_(resize_if_full), position_nesting_(0) {
    }

    ~Text_stream() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    bool open(const char *fname, uint32_t buf_size = Default_buf_size) {

        if (fd_ != -1) {
            ERROR("Can't open file twice\n");
            return false;
        }

        int fd = ::open(fname, O_RDONLY);
        if (fd == -1) {
            ERROR("Can't open file %s error %d\n", fname, error_);
            error_ = errno;
            return false;
        }


        if (!open(fd, buf_size)) {
            return false;
        }
        return true;
    }

    bool open(int fd, uint32_t buf_size = Default_buf_size ) {
        fd_ = fd;
        if (!buf_.init(buf_size)) {
            ERROR("Can't allocate buffer error %d\n", errno);
            ::close(fd_);
            return false;
        }

        return true;
    }

    bool close() {
         if (fd_ != -1) {
            if (::close(fd_) != 0) {
				ERROR("close failed. errno %d\n", errno);
				return false;
			}
   			fd_ = -1;
     	} 
		buf_.close();
        return true;
    }

    bool write_tail(const Char_t *data, uint32_t data_len) {
        if (buf_.free() < data_len) {
			if (resize_if_full_) {
				if (!buf_.resize(buf_.size() + data_len + 1)) {
					return false;
				}
			} else {
	  			return false;
			}
        }

        if (buf_.tail_ >= buf_.head_) {
            auto next_size = buf_.size_ - buf_.tail_;
            auto to_copy = std::min(data_len, next_size);
            
            memcpy((uint8_t *) buf_.buf_ + buf_.tail_, data, to_copy);

            auto remaining = data_len - to_copy;
            if (remaining > 0) {
                memcpy((uint8_t *) buf_.buf_, data + to_copy , remaining);
            }
        } else {
            memcpy((uint8_t *) buf_.buf_ + buf_.tail_, data, data_len ); 
        }

        return buf_.inc_tail( data_len );
    }

    Next_char_value current_char() {

        if (buf_.eof_cursor()) {
            // if out of data then read as much as possible.
            if (buf_.empty()) {

                if (!read_empty()) {
                    // read error
                    return Next_char_value(false,' ');
                }


            } else if (!buf_.full()) {
                      
                if (!read_next()) {
                    // read error
                    return Next_char_value(false,' ');
                }

            } else {
				if (fd_ != -1) {
					if (resize_if_full_) {
						if (!buf_.resize()) {
							ERROR("Failed to resize the buffer\n");
							return Next_char_value(false,' ');
						} 
						if (!read_next()) {
							// read error
							return Next_char_value(false,' ');
						}

					} else {
                		ERROR("buffer is full and can't read any more data\n");
					}
				}
                return Next_char_value(false,' ');
            }
        }         

        if (buf_.empty()) {
            // end of text has been reached.
            return Next_char_value(false,' ');
        }

        if (buf_.eof_cursor()) {
            ERROR("HOW?\n");
            exit(1);
            exit(1);
        }
       auto cur_char = buf_.cursor_char();
       return Next_char_value(true,cur_char);
    }


    Next_char_value next_char() {

		auto rval = current_char();

		if (rval.first) {
				auto cur_char = buf_.cursor_char();
				if (!buf_.inc_cursor()) {
					ERROR("HOW???\n");
					exit(1);

				}
				pos_at_cursor_.next_char(rval.second);
		}
        return rval; 
    }

    Text_position pos_at_cursor() const {
		position_nesting_ += 1;
        return pos_at_cursor_;
    }

    bool backtrack(Text_position pos) {
        if (pos.buffer_pos_ < pos_at_head_ || pos.buffer_pos_ > (pos_at_head_ + buf_.size()) ) {
            ERROR("can't set text position - out of range pos %ld  head_pos %ld size %u\n", pos.buffer_pos_, pos_at_head_, buf_.size());
            return true;
        }
        pos_at_cursor_ = pos;
        buf_.set_cursor( pos_at_cursor_.buffer_pos_ );
		if (position_nesting_ == 0) {
			ERROR("position nesting droppes to negative count. parser is wrong\n");
		}
		dec_position_nesting();
        return true;
    }

	void dec_position_nesting() {
		if (position_nesting_ == 0) {
			ERROR("position nesting droppes to negative count. parser is wrong\n");
		}
		position_nesting_ -= 1;
	}

	// discard all text up until the argument position
    bool move_on(Text_position pos) {

        if (pos.buffer_pos_ < pos_at_head_ || pos.buffer_pos_ > (pos_at_head_ + buf_.size()) ) {
            ERROR("can't set text position - out of range pos %ld  head_pos %ld size %u\n", pos.buffer_pos_, pos_at_head_, buf_.size());
            return true;
        }
        buf_.set_cursor_and_head(pos.buffer_pos_);
        pos_at_cursor_ = pos;
        pos_at_head_ = pos.buffer_pos_; 
        return true;
    }

    int error() { return error_; }

private:
    inline bool read_nextv( iovec  *vec, int num_vec ) {
        ssize_t available = ::readv(fd_, vec, num_vec); 
        if (available == -1) {
            if (errno != EAGAIN) {
                error_ = errno;
                ERROR("read error. error %d\n", error_);
            }
            return false;
        }
        if (available == 0) {
            // select? non blocking fd?
            return false;
        }

        if (!buf_.inc_tail(available)) {
            ERROR("HOW??\n");
			return false;
		}
        return available > 0; 
    }
  
    bool read_next() {

        iovec  vec[2];
        int num_vec = 1;

		if (fd_ == -1) {
			return false;
		}

        if (buf_.cursor_ < buf_.head_) {

            vec[0].iov_base = buf_.buf_ + buf_.cursor_;
            vec[0].iov_len = buf_.head_ - buf_.cursor_ - 1;
        
        } else {

            vec[0].iov_base = buf_.buf_ + buf_.cursor_;
            vec[0].iov_len = buf_.size_ - buf_.cursor_;

            if (buf_.head_ > 0) {
                vec[1].iov_base = buf_.buf_;
                vec[1].iov_len = buf_.head_ - 1;
                num_vec = 2;

            } else {

                vec[0].iov_len -= 1;

            }
        }

        return read_nextv(vec, num_vec);
     }


     bool read_empty() {
        iovec  vec[2];
        int num_vec = 1;

        vec[0].iov_base = buf_.buf_ + buf_.head_;
        vec[0].iov_len = buf_.size_ - buf_.head_;

        if (buf_.head_ > 1) {
           vec[1].iov_base = buf_.buf_;
           vec[1].iov_len = buf_.head_ - 1;
           num_vec = 2;
        }
        if (buf_.head_ == 0) {
           vec[0].iov_len -= 1;
        }
         
        return read_nextv(vec, num_vec);
   }


    int fd_;
    int error_;
    FilePos_t pos_at_head_;
	bool resize_if_full_;
    Text_position  pos_at_cursor_;
    Text_ringbuffer buf_; 
	mutable int position_nesting_;
};

} // namespace pparse

