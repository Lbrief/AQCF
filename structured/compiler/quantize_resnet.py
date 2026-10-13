#!/usr/bin/env python3
"""Recreate fixed Q12 weights and Q24 biases from the bound ResNet checkpoints."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
import torch

CHECKPOINTS = {
    'cifar_resnet18': {'54bde800b187ab6fee769747c5fa0f1e1cf0ea2d49b741cf0bc8a7d18d8fef3b',
                       '6c578feabaef6eb2f0c9587864671321af75dd1c2c89f4a66446493a1588d8ea'},
    'tiny_resnet18': {'19f1d0e94c7c7bc957efdf4706903a9dd5ff4948ab6e5a2c2ae916499b2e6d0c',
                      '5a9ba96acdbd6ee3c1e98c817814a1df683105465c7bca418cfba79e6e93c860'},
}
CIR_BLOCKS = (1, 1, 1, 1, 4, 2, 2, 1, 4, 8, 4, 4, 4, 4, 8, 2, 4, 8, 16)


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def effective_cir_weight(weight, block):
    """Published ILP=False projection followed by exact materialization."""
    co, ci = weight.shape[:2]
    b = block
    if co % b or ci % b:
        raise ValueError('circulant block does not divide channel dimensions')
    tail = tuple(weight.shape[2:])
    blocks = weight.reshape(co // b, b, ci // b, b, *tail)
    blocks = blocks.permute(0, 2, 1, 3, *range(4, blocks.ndim))
    i = torch.arange(b)[:, None]
    d = torch.arange(b)[None, :]
    generator = blocks[:, :, i, (i + d) % b].mean(dim=2).contiguous()
    expanded = generator[:, :, (d - i) % b]
    expanded = expanded.permute(0, 2, 1, 3, *range(4, expanded.ndim))
    return expanded.reshape(co, ci, *tail).contiguous()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=tuple(CHECKPOINTS), required=True)
    parser.add_argument('--checkpoint', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    digest = sha(args.checkpoint)
    if digest not in CHECKPOINTS[args.profile]:
        raise ValueError('checkpoint hash does not match the fixed paper profile')
    torch.set_num_threads(2)
    # The hash-bound original author files also contain argparse metadata.
    # Only that inert standard-library type is allowed; never use unsafe load.
    original = digest in {'6c578feabaef6eb2f0c9587864671321af75dd1c2c89f4a66446493a1588d8ea',
                          '5a9ba96acdbd6ee3c1e98c817814a1df683105465c7bca418cfba79e6e93c860'}
    if original:
        if not hasattr(torch.serialization, 'safe_globals'):
            raise RuntimeError('Original author checkpoint import requires PyTorch 2.6 or newer; tensor-only state files also work on PyTorch 2.3')
        with torch.serialization.safe_globals([argparse.Namespace]):
            state = torch.load(args.checkpoint, map_location='cpu', weights_only=True)
    else:
        state = torch.load(args.checkpoint, map_location='cpu', weights_only=True)
    if 'state_dict' in state:
        state = state['state_dict']
    graph = json.loads((Path(__file__).parent / 'graphs' / (args.profile + '.json')).read_text())
    nodes = [n for n in graph['nodes'] if n['op'] in ('CONV_BN', 'FC')]
    names = []
    for stage in range(1, 5):
        for block in range(2):
            name = f'layer{stage}.{block}'
            names.extend((name + '.conv1', name + '.conv2'))
            if stage > 1 and block == 0:
                names.append(name + '.downsample.0')
    blocks = dict(zip(names, CIR_BLOCKS)) if args.profile == 'cifar_resnet18' else {}
    arrays = {}
    for index, node in enumerate(nodes):
        name = node['name']
        weight = state[name + '.weight'].detach().cpu()
        if name in blocks:
            weight = effective_cir_weight(weight, blocks[name])
        weight = weight.double().numpy().copy()
        if node['op'] == 'CONV_BN':
            bn = node['bn']
            scale = (state[bn + '.weight'].double() /
                     torch.sqrt(state[bn + '.running_var'].double() + 1e-5)).numpy()
            bias = state[bn + '.bias'].double().numpy() - scale * state[bn + '.running_mean'].double().numpy()
            if name + '.bias' in state:
                bias += scale * state[name + '.bias'].double().numpy()
            weight *= scale[:, None, None, None]
        else:
            bias = state[name + '.bias'].double().numpy().copy()
        arrays[f'w{index}'] = np.rint(weight * 4096).astype('<i8')
        arrays[f'b{index}'] = np.rint(bias * (1 << 24)).astype('<i8')
    args.output.mkdir(parents=True, exist_ok=False)
    np.savez_compressed(args.output / 'parameters.npz', **arrays)
    receipt = {'profile': args.profile, 'checkpoint_sha256': digest,
               'arrays_sha256': {name: hashlib.sha256(a.tobytes()).hexdigest() for name, a in arrays.items()},
               'weight_fraction_bits': 12, 'bias_fraction_bits': 24,
               'bn_epsilon': 1e-5, 'training_executed': False}
    if args.profile == 'cifar_resnet18':
        model = args.output / 'model.u64'
        with model.open('xb') as stream:
            stream.write(b'RHCFM001' + struct.pack('<Q', len(nodes)))
            for index, node in enumerate(nodes):
                weight, bias = arrays[f'w{index}'], arrays[f'b{index}']
                if node['op'] == 'CONV_BN':
                    weight = weight.transpose(2, 3, 1, 0).copy()
                stream.write(struct.pack('<2Q', weight.size, bias.size))
                stream.write(weight.tobytes())
                stream.write(bias.tobytes())
        receipt['model_sha256'] = sha(model)
        if receipt['model_sha256'] != '5945be459292c36526a7290a46f6a49f7b451ea1bf13b2e9342487a5f0dc7fbc':
            raise ValueError('export differs from the retained integer model; output preserved for inspection')
    (args.output / 'quantization.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': 'QUANTIZED_NO_INFERENCE', 'output': str(args.output)}))


if __name__ == '__main__':
    main()
