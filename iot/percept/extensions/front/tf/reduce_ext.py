# Copyright (C) 2018-2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from extensions.ops.ReduceOps import ReduceProd, ReduceAnd, ReduceMax, ReduceMean, ReduceSum, ReduceL2
from mo.front.extractor import FrontExtractorOp
from mo.graph.graph import Node


class AllFrontExtractor(FrontExtractorOp):
    op = 'All'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        keep_dims = node.pb.attr['keep_dims'].b
        ReduceAnd.update_node_stat(node, {'keep_dims': keep_dims})
        return cls.enabled


class MaxFrontExtractor(FrontExtractorOp):
    op = 'Max'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        ReduceMax.update_node_stat(node, {'keep_dims': node.pb.attr['keep_dims'].b})
        return cls.enabled


class MeanExtractor(FrontExtractorOp):
    op = 'Mean'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        ReduceMean.update_node_stat(node, {'keep_dims': node.pb.attr["keep_dims"].b})
        return cls.enabled


class ProdFrontExtractor(FrontExtractorOp):
    op = 'Prod'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        ReduceProd.update_node_stat(node, {'keep_dims': node.pb.attr["keep_dims"].b})
        return cls.enabled


class SumFrontExtractor(FrontExtractorOp):
    op = 'Sum'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        ReduceSum.update_node_stat(node, {'keep_dims': node.pb.attr["keep_dims"].b})
        return cls.enabled


class EuclideanNormFrontExtractor(FrontExtractorOp):
    op = 'EuclideanNorm'
    enabled = True

    @classmethod
    def extract(cls, node: Node):
        ReduceL2.update_node_stat(node, {'keep_dims': node.pb.attr["keep_dims"].b})
        return cls.enabled
