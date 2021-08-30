#include "InferenceIOInfo.hpp" // for vpual::infer::IOInfo

namespace vpual {
namespace infer {

IOInfo::IOInfo():
    inputs_count {0},
    outputs_count {0},
    input_names {},
    output_names {},
    input_shapes {},
    output_shapes {},
    output_offsets {},
    output_lengths {},
    total_output_length {0}
{
}

IOInfo::IOInfo(std::vector<std::string> input_names,
               std::vector<IOInfo::TensorShape> input_shapes,
               std::vector<std::string> output_names,
               std::vector<IOInfo::TensorShape> output_shapes):
        input_names {input_names},
        input_shapes {input_shapes},
        output_names {output_names},
        output_shapes {output_shapes},
        output_offsets {},
        output_lengths {},
        total_output_length {0}
{

    if (input_names.size() != input_shapes.size()) {
        std::abort();
    } else if (output_names.size() != output_shapes.size()) {
        std::abort();
    }

    inputs_count = input_names.size();
    outputs_count = output_names.size();
}


bool IOInfo::is_empty(void) const {
    return !(inputs_count && outputs_count);
}

IOInfo::TensorShape::TensorShape(Layout layout,
    DataType data_type,
    std::vector<std::uint32_t> dims):
    layout {layout},
    data_type {data_type},
    dims {dims}
{
    num_dims = dims.size();
}

bool operator==(const IOInfo::TensorShape& lhs,
        const IOInfo::TensorShape& rhs) noexcept {
    return lhs.layout == rhs.layout &&
           lhs.data_type == rhs.data_type &&
           lhs.dims == rhs.dims;
}

bool operator==(const IOInfo& lhs, const IOInfo& rhs) noexcept {
    return lhs.input_names == rhs.input_names &&
           lhs.input_shapes == rhs.input_shapes &&
           lhs.output_names == rhs.output_names &&
           lhs.output_shapes == rhs.output_shapes;
}

} // namespace infer
} // namespace vpual
