#!/usr/bin/env python3
"""Compile structured study integer parameters into the exact public layouts."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import numpy as np
import coefficient_program as compiler
import spatial_planner as planner
import program_writer as writer

ROOT = Path(__file__).parent
EXPECTED = {
    'cifar_resnet18': ('eb1d99fd073c017d076a1cf9fbf5a58f32ed1abf98b5df4a825abb1a8cce99ce', '86b6ea505e7fc598554323da71d7affe433358d98079b0e9432f99b2d23e6c08'),
    'tiny_resnet18': ('c09b6da1cd5e1664989cd538c0cd6d215f2b35237d7572962fc4e7dfa51b9a90', '071688251d4c08e6ef0037e1074bfc537649cd9eee62aeb101bf5e25c05404ac'),
    'cifar_mobilenetv2': ('9a535b62c1706bb324ff8df250b7efe2c8633a867245d65aa58035185db348ef', '439db0bb341a8bcf2aa0debcf6c5cefa237ca6673d4c07cdcbaf8ba0ec33b930'),
}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def load_mobile(path, graph):
    if sha(path) != '93f954f0bba8559380a8c8159c4a8b5f7211164015fabe2624fb735169067032':
        raise ValueError('MobileNet model must match the fixed BN/IR-compatible integer export')
    params = {}
    with path.open('rb') as stream:
        if stream.read(8) != b'PCNMBV21' or struct.unpack('<Q', stream.read(8))[0] != 53:
            raise ValueError('wrong MobileNet model header')
        for node in graph['nodes']:
            if node['op'] not in ('CONV', 'CIRCONV', 'DEPTHWISE', 'FC'):
                continue
            ident, nw, nb = struct.unpack('<3Q', stream.read(24))
            if ident != node['id'] or nw != math.prod(node['weight_shape']) or nb != node['Co']:
                raise ValueError('MobileNet coefficient record shape mismatch')
            weight = np.frombuffer(stream.read(nw * 8), dtype='<i8').copy().reshape(node['weight_shape'])
            bias = np.frombuffer(stream.read(nb * 8), dtype='<i8').copy()
            params[ident] = (weight, bias)
        if stream.read(1):
            raise ValueError('trailing model bytes')
    return params


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=tuple(EXPECTED), required=True)
    parser.add_argument('--parameters', type=Path, required=True,
                        help='ResNet: parameters.npz; MobileNet: fixed PCNMBV21 model.bin')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    graph = json.loads((ROOT / 'graphs' / (args.profile + '.json')).read_text())
    mobile = args.profile == 'cifar_mobilenetv2'
    params = load_mobile(args.parameters, graph) if mobile else np.load(args.parameters, allow_pickle=False)
    affine_ops = ('CONV', 'CIRCONV', 'DEPTHWISE', 'FC') if mobile else ('CONV_BN', 'FC')
    nodes = [node for node in graph['nodes'] if node['op'] in affine_ops]
    plans = json.loads((ROOT / 'resnet_placement_plans.json').read_text())
    cifar_graph = json.loads((ROOT / 'graphs/cifar_resnet18.json').read_text())
    cifar_nodes = [n for n in cifar_graph['nodes'] if n['op'] in ('CONV_BN', 'FC')]
    active = {}

    def choose(planner, ci, co, h, w, ho, wo, k, s, pad, cache):
        plan = active.get('plan')
        if plan is None:
            return writer.choose(planner, ci, co, h, w, ho, wo, k, s, pad, cache)
        for tile in plan['tiles']:
            placement = tile['placement']
            if planner.placement_certificate(tile['geometry'], placement['J'], placement['F']) != placement['certificate']:
                raise ValueError('placement certificate does not match public geometry')
        return plan

    compiler.select_plan = writer.choose if mobile else choose
    args.output.mkdir(parents=True, exist_ok=False)
    rows, cache = [], {}
    program_path, layout_path = args.output / 'program.bin', args.output / 'layout.bin'
    with program_path.open('xb') as program, layout_path.open('xb') as layout:
        program.write(b'AQCFGPS1')
        layout.write(b'AQCFCL01')
        for stream in (program, layout):
            writer.words(stream, 1, 8192, 1152921504606830593, len(nodes))
        for index, node in enumerate(nodes):
            if mobile:
                weight, bias = params[node['id']]
                scale = 24 if node['id'] in {19, 54, 62, 85} else 12
                compiled, plan = writer.compile_affine(compiler, planner, index, node, weight, bias, scale, cache)
            else:
                weight, bias = params[f'w{index}'], params[f'b{index}']
                if weight.dtype != np.int64 or bias.dtype != np.int64:
                    raise ValueError('integer parameter arrays are required')
                same = all(node.get(key) == cifar_nodes[index].get(key)
                           for key in ('op', 'shape', 'in_shape', 'kernel', 'stride', 'padding'))
                active['plan'] = plans[index]['plan'] if same else None
                h, w, ci = node['in_shape']
                ho, wo, co = node['shape'] if node['op'] == 'CONV_BN' else (1, 1, graph['classes'])
                if node['op'] == 'FC':
                    weight = weight.reshape(co, ci, 1, 1)
                stub = dict(shape=[ho, wo, co], k=node.get('kernel', 1), stride=node.get('stride', 1), pad=node.get('padding', 0))
                compiled, plan = compiler.dense_program(np, planner, index, stub, [1, ci, h, w], weight, np.zeros(co, dtype=np.int64), cache)
                for _, query in compiled['outputs']:
                    query[:, 2] = bias[query[:, 0] // (ho * wo)]
            counts = writer.validate_write(compiled, program, layout)
            rows.append({'affine': index, 'name': node['name'], 'counts': counts})
            print(f'compiled {index + 1}/{len(nodes)} {node["name"]}', flush=True)
    actual = sha(program_path), sha(layout_path)
    receipt = {'profile': args.profile, 'program_sha256': actual[0], 'layout_sha256': actual[1],
               'matches_retained_paper_assets': actual == EXPECTED[args.profile],
               'layers': rows, 'inference_executed': False}
    (args.output / 'compilation.json').write_text(json.dumps(receipt, indent=2) + '\n')
    if actual != EXPECTED[args.profile]:
        raise ValueError('compiled files differ from retained profile hashes; inspect compilation.json')
    print('EXACT_PROGRAM_AND_LAYOUT_MATCH')


if __name__ == '__main__':
    main()
