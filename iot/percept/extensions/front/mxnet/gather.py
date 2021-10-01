# Copyright (C) 2018-2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from extensions.ops.gather import Gather
from mo.front.common.partial_infer.utils import int64_array
from mo.front.common.replacement import FrontReplacementOp
from mo.front.tf.graph_utils import create_op_with_const_inputs
from mo.graph.graph import Graph


class GatherFrontReplacer(FrontReplacementOp):
    op = 'Embedding'
    enabled = True

    def replace_sub_graph(self, graph: Graph, match: dict):
        node = match['op']

        gather_node = create_op_with_const_inputs(graph, Gather, {2: int64_array(0)},
                                                  {'name': node.soft_get('name', node.id) + '/embedding_'})

        node.in_port(0).get_connection().set_destination(gather_node.in_port(1))
        node.in_port(1).get_connection().set_destination(gather_node.in_port(0))
        node.out_port(0).get_connection().set_source(gather_node.out_port(0))
