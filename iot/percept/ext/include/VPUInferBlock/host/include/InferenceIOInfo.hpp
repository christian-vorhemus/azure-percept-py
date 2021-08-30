#ifndef _VPUAL_INFERENCE_SHAPE_INFO_HPP_
#define _VPUAL_INFERENCE_SHAPE_INFO_HPP_

#include <vector> // for std::vector
#include <string> // for std::string
#include <cstdint> // for std::uint32_t

namespace vpual {
namespace infer {

struct IOInfo
{
    // tensor layout
    enum Layout : std::uint32_t
    {
        NHWC = 0x4213,
        NHCW = 0x4231,
        NCHW = 0x4321,
        HWC = 0x213,
        CHW = 0x321,
        WHC = 0x123,
        HCW = 0x231,
        WCH = 0x132,
        CWH = 0x312,
        NC = 0x43,
        CN = 0x34,
        C = 0x3,
        H = 0x2,
        W = 0x1,
    };

    // tensor data dype
    enum DataType : std::uint32_t
    {
        FP16 = 0,
        U8F = 1,
        INT = 2,
        FP32 = 3,
        I8 = 4,
    };

    // tensor shape structure
    struct TensorShape
    {
        TensorShape() = default;
        TensorShape(Layout layout,
                    DataType data_type,
                    std::vector<std::uint32_t> dims);

        Layout layout;
        DataType data_type;
        std::uint32_t num_dims;
        std::vector<std::uint32_t> dims {};
    };

    IOInfo();

    IOInfo(std::vector<std::string> input_names,
           std::vector<TensorShape> input_shapes,
           std::vector<std::string> output_names,
           std::vector<TensorShape> output_shapes);


    std::uint32_t inputs_count;  // number of input layers
    std::uint32_t outputs_count; // number of output layers
    std::vector<std::string>
        input_names; // array with the name of each input layer
    std::vector<std::string>
        output_names; // array with the name of each output layer
    std::vector<TensorShape>
        input_shapes; // array with the shape of each input layer
    std::vector<TensorShape>
        output_shapes; // array with the shape of each output layer
    std::vector<std::uint32_t>
        output_offsets; // array with the offset of each output tensor
    std::vector<std::uint32_t>
        output_lengths; // array with the length of each output tensor
    std::uint32_t total_output_length;

    bool is_empty(void) const;
};

bool operator==(const IOInfo::TensorShape& lhs,
        const IOInfo::TensorShape& rhs) noexcept;

bool operator==(const IOInfo& lhs, const IOInfo& rhs) noexcept;

} // namespace infer
} // namespace vpual

#endif // _VPUAL_INFERENCE_SHAPE_INFO_HPP_
