#ifndef VPUAL_MESSAGE_H_
#define VPUAL_MESSAGE_H_

#include <vector> // for std::vector
#include <cstdint> // for std::uint8_t

#ifdef MA2X8X
#include <mvOtMemory.h>
#endif

namespace vpual {
namespace core {

// Vpual Command Header Structure.
struct CommandHeader {
	// Type of message.
    enum message
    {
        DECODER_CREATE = 1,  // Create a decoder.
        DECODER_DESTROY = 2, // Destroy a decoder.
        DECODER_DECODE = 3,  // Call a decoder's custom "Decode" method.
    };

    uint32_t magic;    // Magic value.
    uint32_t checksum; // Checksum of payload?
    uint32_t msg_id;   // ID of message.

    uint32_t length;  // Length of payload.
    uint32_t stubID;  // ID of stub/decoder.
    message type;     // Type of message.
};

/**
 * Message Class
 *
 * This class handles serialization and deserialization (read/write) from a buffer.
 * Message manages its own internal buffer, allocated and deallocated using RAII mechanics
 *
 */
class Message {
public:

#ifdef MA2X8X
    typedef mv::ot::memory::cache_aligned_vector<std::uint8_t> container_type;
#else
    typedef std::vector<std::uint8_t>      container_type;
#endif

    typedef container_type::value_type     value_type;
    typedef container_type::iterator       iterator;
    typedef container_type::const_iterator const_iterator;
private:
    // Serialized data in contiguous blocks handled by vector
    container_type sdata_;

    // Current write position
    std::size_t write_pos_;

    // Current read position
    std::size_t read_pos_;
public:
    // Default constructible
    // FIXME: replace with large prealloc for now, to avoid cache problems at realloc
    //Message() = default;
    Message();

    // Constructible with a reserved size
    // @param size - size to be reserved in the container
    // Note: The reserve semantics are the same as those of std::vector
    Message(const std::uint32_t len);

    // Constructible with arbitrary data
    // @param idata - input data to constructor
    // @param size  - input data size
    Message(std::uint8_t const* idata, std::uint32_t size);

    // Constructible with initializer_list
    Message(const std::initializer_list<value_type>& idata);

    // Get front element of the serialized data
    value_type& front();
    const value_type& front() const;

    // Get back element of the serialized data
    value_type& back();
    const value_type& back() const;

    // Serialized data is subscriptible
    value_type& operator[](std::size_t pos);
    const value_type& operator[](std::size_t pos) const;

    // Get iterator to the beginning of the serialized data
    iterator begin() noexcept;
    const_iterator begin() const noexcept;

    // Get iterator to one-plus-the-last element of the serialized data
    iterator end() noexcept;
    const_iterator end() const noexcept;

    // Clear serialized data
    void clear() noexcept;

    // Handles serialization
    // @param idata - input data
    // @param size  - input data size
    //
    // @return - amount of unused allocated data or -1 on error
    int serialize(const void *const idata, std::uint32_t size);

    // Handles serialization
    // @param idata - input data
    // @param size  - input data size
    //
    // @return - same as untemplated version
    template <typename T>
    int serialize(T idata) {
        static_assert(std::is_pod<T>::value,
                "Message::serialize support only POD types");
        return serialize(&idata,sizeof(idata));
    }

    // Handled deserialization
    // @param odata - output buffer for deserialization
    // @param size  - output buffer size
    int deserialize(void *const odata, std::uint32_t size);

    // Reserve space in the underlying container
    // @param size - size to be reserved in the container
    // Note: semantics are the same as those of the underlying container
    void reserve(std::size_t size);

    // Serialized data size in bytes (does not track write pointer)
    std::size_t size(void) const { return sdata_.size(); }

    // Poiter to underlying data getter
    value_type* data() { return sdata_.data(); }
    const value_type *data() const { return sdata_.data(); }

    // Get index of current read position
    //
    // @return - read pointer
    std::size_t get_read_index(void);

    // Set index of current read position
    //
    // @param new_index - new position of read pointer
    void set_read_index(std::size_t new_index);

    // ************************************************************* //
    // Plugin Migration API
    // ************************************************************* //

	// Create a buffer based on provided length.
	//
	// @param len the length (bytes) of the buffer to create.
	//
    // Added for plugin migration, equivalent to reserve() method.
    void create(const std::uint32_t len);

    // Get index position of the write pointer
    //
    // @return write pointer value.
    //
    // Added for plugin migration
    std::uint32_t len(void) const;

	// Get the buffer.
	//
	// @return start address of buffer.
	//
    // Added for plugin migration, equivalent to data() method.
    std::uint8_t* dat(void) const;

    // Set a new write position for the buffer.
    //
    // @param new_wp the new write position (bytes) for the buffer.
    //
    // Add for plugin migration
    void set_len(std::uint32_t new_wp);
};

} // namespace core
} // namespace vpual

typedef vpual::core::Message VpualMessage;

#endif // VPUAL_MESSAGE_H_
