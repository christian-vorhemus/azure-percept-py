# Copyright (C) 2018-2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from mo.front.extractor import FrontExtractorOp
from mo.front.mxnet.extractors.utils import get_mxnet_layer_attrs
from mo.ops.softmax import Softmax


class SoftmaxFrontExtractor(FrontExtractorOp):
    op = 'softmax'
    enabled = True

    @classmethod
    def extract(cls, node):
        attrs = get_mxnet_layer_attrs(node.symbol_dict)

        update_attrs = {
            'type': 'SoftMax',
            'axis': attrs.int("axis", -1),
            'temperature': attrs.float('temperature', 1.0)
        }

        # update the attributes of the node
        Softmax.update_node_stat(node, update_attrs)
        return cls.enabled
