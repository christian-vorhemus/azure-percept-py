# Copyright (C) 2018-2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from mo.graph.graph import Node, Graph
from mo.ops.op import Op
from mo.utils.error import Error
from mo.utils.utils import refer_to_faq_msg


class BoxNms(Op):
    ''' It is assumed that there is no equivalent of this op in IE.
    '''
    op = '_contrib_box_nms'

    def __init__(self, graph: Graph, attrs: dict):
        mandatory_props = {
            'type': None,
            'op': __class__.op,
            'coord_start': 2,
            'force_suppress': False,
            'id_index': 0,
            'overlap_thresh': 0.45,
            'score_index': 1,
            'topk': 400,
            'valid_thresh': 0.01,
            'infer': __class__.infer
        }
        super().__init__(graph, mandatory_props, attrs)

    def supported_attrs(self):
        return [
            'coord_start',
            'force_suppress',
            'id_index',
            'overlap_thresh',
            'score_index',
            'topk',
            'valid_thresh',
        ]

    @staticmethod
    def infer(node: Node):
        raise Error(
            "Operation _contrib_box_nms not not supported. " +
            "For gluoncv ssd topologies use cmd parameter: '--enable_ssd_gluoncv' " +
            refer_to_faq_msg(102))
