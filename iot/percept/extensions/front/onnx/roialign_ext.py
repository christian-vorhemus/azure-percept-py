# Copyright (C) 2018-2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from extensions.ops.roialign import ROIAlign
from mo.front.extractor import FrontExtractorOp
from mo.front.onnx.extractors.utils import onnx_attr


class ROIAlignExtractor(FrontExtractorOp):
    op = 'ROIAlign'
    enabled = True

    @classmethod
    def extract(cls, node):
        mode = onnx_attr(node, 'mode', 's', default=b'avg').decode()
        output_height = onnx_attr(node, 'output_height', 'i', default=1)
        output_width = onnx_attr(node, 'output_width', 'i', default=1)
        sampling_ratio = onnx_attr(node, 'sampling_ratio', 'i', default=0)
        spatial_scale = onnx_attr(node, 'spatial_scale', 'f', default=1.0)

        ROIAlign.update_node_stat(node, {'pooled_h': output_height, 'pooled_w': output_width,
                                         'sampling_ratio': sampling_ratio, 'spatial_scale': spatial_scale,
                                         'mode': mode})
        return cls.enabled
