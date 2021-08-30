/*
 * mxIfInferBlock.h
 *
 *  Created on: Apr 6, 2020
 *      Author: apalfi
 */

#ifndef HOST_MXIF_MXIFINFERBLOCK_H_
#define HOST_MXIF_MXIFINFERBLOCK_H_

// Includes
// -------------------------------------------------------------------------------------
#include <map>
#include <vector> // for std::vector
#include <string> // for std::string
#include <memory> // for std::unique_ptr
#include <utility> // for std::pair

#include <InferenceIOInfo.hpp> // for vpual::infer::IOInfo::TensorShape

#include "mxIfMemoryHandle.h"

// Classes
// -------------------------------------------------------------------------------------
namespace mxIf
{
    using TensorShape = vpual::infer::IOInfo::TensorShape;

    struct RAW_BLOB {};
    struct PREPROCESS_FULL {};

    struct PREPROCESS_ROI {
        // Roi structure
        uint32_t x; // x of the top-left corner
        uint32_t y; // y of the top-left corner
        uint32_t width; // width of the rectangle
        uint32_t height; // height of the rectangle

        PREPROCESS_ROI();
        PREPROCESS_ROI(uint32_t x,uint32_t y,uint32_t width,uint32_t height);
    };

    class InferIn {
    public:
        MemoryHandle handle_;
        enum {RAW,ROI,FULL} type_;
        PREPROCESS_ROI roi_;
        InferIn(MemoryHandle handle,RAW_BLOB);
        InferIn(MemoryHandle handle,PREPROCESS_FULL);
        InferIn(MemoryHandle handle,PREPROCESS_ROI roi);
    };

    class InferBlock
    {
    public:
        InferBlock(const std::string& blobfile);

        void enable_latency_counter(bool status);
        void enable_throughput_counter(bool status);

        std::map<std::string, MemoryHandle> GetNextOutput();

        std::pair<std::vector<std::string>,std::vector<std::string>>
            get_info();

        std::vector<TensorShape> get_input_shapes();
        std::vector<TensorShape> get_output_shapes();

        void Enqueue(const std::map<std::string,mxIf::InferIn>& input);

        InferBlock(const InferBlock&) = delete;

        // Explicitly defaulted in implementation file
        InferBlock(InferBlock&& infer);
        InferBlock& operator=(const InferBlock&) = default;

        // Explicitly defaulted in implementation file
        ~InferBlock();
    private:
        class InferBlockImpl;
        std::unique_ptr<InferBlockImpl> pinfer_;
    };
} // namespace mxIf

#endif /* HOST_MXIF_MXIFINFERBLOCK_H_ */
