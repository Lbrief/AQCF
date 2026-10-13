#!/usr/bin/env python3
"""Prepare the fixed dataset input or synthetic check input for each profile."""
import argparse
import hashlib
import json
from pathlib import Path
import pickle
import struct
import numpy as np


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('cifar_resnet18', 'tiny_resnet18', 'cifar_mobilenetv2'), required=True)
    parser.add_argument('--mode', choices=('PUBLIC', 'REAL'), required=True)
    sources = parser.add_mutually_exclusive_group()
    sources.add_argument('--cifar-test', type=Path, help='Official cifar-100-python/test file')
    sources.add_argument('--raw-npy', type=Path, help='RGB CHW uint8 array for the fixed CIFAR image')
    sources.add_argument('--tiny-image', type=Path, help='Tiny ImageNet val_9197.JPEG')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    record = json.loads((Path(__file__).parent / 'reference_scores.json').read_text())[args.profile]
    if args.mode == 'PUBLIC':
        if any((args.cifar_test, args.raw_npy, args.tiny_image)):
            parser.error('PUBLIC inputs are deterministic; do not provide a dataset image')
        n = int(np.prod(record['shape']))
        if args.profile == 'tiny_resnet18':
            values = (np.arange(n, dtype=np.int64) % 17 - 8) * 64
        elif args.profile == 'cifar_resnet18':
            values = (np.arange(n, dtype=np.int64) * 29) % 24577 - 12288
        else:
            values = (np.arange(n, dtype=np.int64) * 73) % 8193 - 4096
    elif args.profile == 'tiny_resnet18':
        if not args.tiny_image:
            parser.error('Tiny REAL input requires --tiny-image')
        if sha(args.tiny_image) != record['image_sha256']:
            raise ValueError('Tiny ImageNet image differs from the fixed study image')
        from PIL import Image
        import torch
        from torchvision import transforms
        transform = transforms.Compose([
            transforms.Resize(71, interpolation=transforms.InterpolationMode.BICUBIC),
            transforms.CenterCrop(64), transforms.ToTensor(),
            transforms.Normalize([.485, .456, .406], [.229, .224, .225])])
        tensor = transform(Image.open(args.tiny_image).convert('RGB')).double()
        values = torch.round(tensor * 4096).to(torch.int64).numpy()
    else:
        if args.raw_npy:
            raw = np.load(args.raw_npy, allow_pickle=False)
        elif args.cifar_test:
            # Decode only the hash-bound original dataset pickle.
            if sha(args.cifar_test) != '4b67687d9933c4db8f0831104447f15b93774f4f464bd0516f0f0f2ac83b7864':
                raise ValueError('CIFAR test file differs from the recorded official release')
            with args.cifar_test.open('rb') as stream:
                dataset = pickle.load(stream, encoding='bytes')
            index = record['image_index']
            if dataset[b'fine_labels'][index] != record['label']:
                raise ValueError('CIFAR label differs from the fixed sample')
            raw = np.asarray(dataset[b'data'][index], dtype=np.uint8).reshape(3, 32, 32)
        else:
            parser.error('CIFAR REAL input requires --cifar-test or --raw-npy')
        if raw.dtype != np.uint8 or raw.shape != (3, 32, 32):
            raise ValueError('expected RGB CHW uint8 array')
        mean = np.array([.5071, .4867, .4408], dtype=np.float32)[:, None, None]
        std = np.array([.2675, .2565, .2761], dtype=np.float32)[:, None, None]
        normalized = (raw.astype(np.float32) / np.float32(255) - mean) / std
        if args.profile == 'cifar_mobilenetv2':
            values = np.rint(normalized.astype(np.float64) * 4096).astype('<i8')
        else:
            values = np.rint(normalized * np.float32(4096)).astype('<i8')
    raw_input = np.asarray(values, dtype='<i8').reshape(record['shape']).tobytes()
    expected = record['references'][args.mode]
    if hashlib.sha256(raw_input).hexdigest() != expected['input_sha256']:
        raise ValueError('preprocessed input differs; check the image and preprocessing versions')
    scores = struct.pack('<' + 'q' * record['classes'], *expected['scores'])
    if hashlib.sha256(scores).hexdigest() != expected['reference_sha256']:
        raise ValueError('reference score metadata is inconsistent')
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'input.i64').write_bytes(raw_input)
    (args.output / 'reference.i64').write_bytes(scores)
    (args.output / 'input.json').write_text(json.dumps({
        'profile': args.profile, 'mode': args.mode, 'shape_nchw': record['shape'],
        'input_sha256': expected['input_sha256'], 'reference_sha256': expected['reference_sha256'],
        'reference_scope': 'retained independent clear integer scores, not a new inference run',
        'image_index': record.get('image_index') if args.mode == 'REAL' else None,
        'image_filename': record.get('image_filename') if args.mode == 'REAL' else None,
    }, indent=2) + '\n')
    print(f'INPUT_AND_REFERENCE_HASHES_VERIFIED: {args.output}')


if __name__ == '__main__':
    main()
