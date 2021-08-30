#include "VpualMessage.h"

#include "secure_functions.h" // for memcpy_s

#include <cstring> // for memcpy_s (when not provided)
#include <cassert> // for assert

#define MVLOG_UNIT_NAME VpualMessage
#include <mvLog.h>

namespace vpual {
namespace core {

Message::Message() : write_pos_{0}, read_pos_{0} {
    // FIXME: large reserve, to avoid realloc
    sdata_.reserve(2048);
}

Message::Message(std::uint32_t size) : write_pos_{0}, read_pos_{0} {
    sdata_.reserve(size);
}

Message::Message(uint8_t const* data, const std::uint32_t len) :
    write_pos_{0},
    read_pos_{0}
{
    const auto* dptr = reinterpret_cast<const value_type*>(data);
    sdata_ = container_type(dptr,dptr + len);
}

Message::Message(const std::initializer_list<std::uint8_t>& data):
        sdata_{data},
        write_pos_{0},
        read_pos_{0}
{}

Message::value_type& Message::front() { return sdata_.front(); }
const Message::value_type& Message::front() const { return sdata_.front(); }

Message::value_type& Message::back() { return sdata_.back(); }
const Message::value_type& Message::back() const { return sdata_.back(); }

Message::value_type& Message::operator[](std::size_t pos) { return sdata_[pos]; }
const Message::value_type& Message::operator[](std::size_t pos) const {
    return sdata_[pos];
}

void Message::clear() noexcept {
    write_pos_ = 0;
    read_pos_ = 0;
    sdata_.clear();
}

int Message::serialize(const void *const idata, std::uint32_t size) {
    if (!idata) {
        mvLog(MVLOG_ERROR, "Argument must not be NULL");
        return -1;
    }

    // Reallocate memory for write
    if (write_pos_ > sdata_.size()) {
        sdata_.resize(write_pos_ + size);
    }

    // Cast to serialization type
    auto* write_data = reinterpret_cast<const value_type *const>(idata);

    // Overwriting write length
    std::size_t overwrite_len = std::min((std::size_t)size,
            sdata_.size() - write_pos_);

    // Overwrite existing contents
    memcpy_s(&sdata_[write_pos_], sdata_.size() - write_pos_, write_data,
            overwrite_len);

    // Check if there are elements to append
    if (overwrite_len < size) {
        sdata_.insert(sdata_.end(), write_data + overwrite_len, write_data + size);
    }

    // Update write position
    write_pos_ = sdata_.size();

    // return remaining size
    return sdata_.size() - write_pos_;
}

int Message::deserialize(void * odata, std::uint32_t size) {
    if (!odata) {
        mvLog(MVLOG_ERROR, "Argument must not be NULL");
        return -1;
    }

    // Check that there is enough data
    if (write_pos_ < (read_pos_ + size)) {
        mvLog(MVLOG_ERROR, "Not enough data to read from VPUAL Message!");
        return write_pos_ - read_pos_ - size;
    }

    // Reinterpret output data pointer to value_type for deserialization
    value_type* tmp = reinterpret_cast<value_type*>(odata);

    // Copy data and erase
    memcpy_s(tmp, size, sdata_.data() + read_pos_,size);

    // Update read position
    read_pos_ += size;

    return size; // return the size in bytes of the deserialized elements
}

void Message::set_read_index(std::size_t new_pos) {
    read_pos_ = new_pos;
}

std::size_t Message::get_read_index(void) {
    return read_pos_;
}

// ***************************************************************
// Plugin Migration API
// ***************************************************************
void Message::reserve(std::size_t size) { sdata_.reserve(size); }

void Message::create(const std::uint32_t len) {
    assert(sdata_.empty()); // check that memory is empty
    sdata_.resize(len);
    write_pos_ = 0;
}

std::uint32_t Message::len(void) const { return write_pos_; }

std::uint8_t* Message::dat(void) const { return (std::uint8_t*)sdata_.data(); }

void Message::set_len(std::uint32_t new_wp) { write_pos_ = new_wp; }

} // namespace core
} // namespace vpual

