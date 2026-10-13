#!/usr/bin/env python3
"""Run two role processes and compare every final score to a supplied reference.

This local reproduction helper is not an OS isolation boundary. It deliberately
does not contact a cloud host, download data, or select an experimental image.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import queue
import secrets
import shutil
import subprocess
import threading
import time


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--layout', type=Path, required=True)
    parser.add_argument('--program', type=Path, required=True)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--mode', choices=('PUBLIC', 'REAL'), required=True,
                        help='PUBLIC: synthetic input; REAL: a dataset image')
    parser.add_argument('--classes', type=int, choices=(100, 200), required=True)
    parser.add_argument('--port', type=int, default=46901)
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('--cpus', help='Two allowed Linux CPU IDs, server,client; default 0,2')
    parser.add_argument('--check-only', action='store_true')
    args = parser.parse_args()
    paths = {key: getattr(args, key).resolve(strict=True)
             for key in ('binary', 'layout', 'program', 'input', 'reference')}
    if not 45000 <= args.port < 49000:
        parser.error('--port must be between 45000 and 48999')
    if paths['reference'].stat().st_size != args.classes * 8:
        parser.error('reference must contain one little-endian signed int64 per class')
    if paths['input'].stat().st_size not in (3 * 32 * 32 * 8, 3 * 64 * 64 * 8):
        parser.error('input must be a 32x32 or 64x64 RGB CHW signed-int64 tensor')
    bound = {key: sha256(path) for key, path in paths.items()}
    if args.check_only:
        print(json.dumps({'status': 'FILES_VALIDATED_NO_PROTOCOL_EXECUTED',
                          'sha256': bound}, indent=2))
        return
    if os.name != 'posix' or not hasattr(os, 'sched_getaffinity'):
        parser.error('execution requires Linux; --check-only is portable')
    try:
        cpus = [int(value) for value in (args.cpus or '0,2').split(',')]
    except ValueError:
        parser.error('--cpus requires two integer IDs, e.g. 0,2')
    if len(cpus) != 2 or len(set(cpus)) != 2 or not set(cpus).issubset(os.sched_getaffinity(0)):
        parser.error('--cpus must name two distinct allowed Linux CPU IDs')
    if not args.output:
        parser.error('--output is required for execution')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / 'inputs').mkdir()
    roles = []
    events: queue.Queue = queue.Queue()
    run_id = secrets.token_hex(32)
    environment = dict(os.environ, AQCF_CPU_SERVER=str(cpus[0]), AQCF_CPU_CLIENT=str(cpus[1]))
    try:
        for party, asset_name in ((1, 'program'), (2, 'input')):
            own = output / 'inputs' / f'party_{party}.bin'
            shutil.copyfile(paths[asset_name], own)
            proof = Path(str(own) + '.proof')
            proof.write_text(bound['layout'] + '\n' + bound[asset_name] + '\n' + run_id + '\n')
            command = [str(paths['binary']), str(party), str(args.port), '1',
                       str(paths['layout']), str(own), str(output / f'party_{party}'),
                       args.mode, sha256(proof)]
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT, text=True, bufsize=1, env=environment)
            roles.append(process)

            def read_role(proc=process, number=party):
                with (output / f'party_{number}.log').open('w', encoding='utf8') as log:
                    for line in proc.stdout:
                        log.write(line)
                        log.flush()
                        events.put((number, line.strip()))
                events.put((number, None))

            threading.Thread(target=read_role, daemon=True).start()
        deadline = time.monotonic() + args.timeout
        observed = {'ASSETS_READY': set(), 'READY': set()}
        sent = set()
        exited = set()
        while len(exited) < 2:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError('pair did not finish within the requested timeout')
            number, event = events.get(timeout=remaining)
            if event is None:
                exited.add(number)
                continue
            if event in observed:
                observed[event].add(number)
                if len(observed[event]) == 2 and event not in sent:
                    for process in roles:
                        process.stdin.write('C' if event == 'ASSETS_READY' else 'G')
                        process.stdin.flush()
                    sent.add(event)
        codes = [process.wait() for process in roles]
        if codes != [0, 0]:
            raise RuntimeError(f'role exit codes {codes}; inspect the retained logs')
        actual = output / 'party_2' / f'FINAL{args.classes}.i64'
        expected_bytes = paths['reference'].read_bytes()
        actual_bytes = actual.read_bytes()
        if len(actual_bytes) != len(expected_bytes):
            raise RuntimeError('final score count differs from reference')
        mismatches = sum(actual_bytes[i:i+8] != expected_bytes[i:i+8]
                         for i in range(0, len(actual_bytes), 8))
        report = {'status': 'PASS' if mismatches == 0 else 'FAIL',
                  'mode': args.mode, 'classes': args.classes,
                  'full_score_mismatches': mismatches, 'source_assets_sha256': bound,
                  'server_cpu': cpus[0], 'client_cpu': cpus[1],
                  'server': json.loads((output / 'party_1/RESULT.json').read_text()),
                  'client': json.loads((output / 'party_2/RESULT.json').read_text())}
        (output / 'comparison.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({'status': report['status'], 'full_score_mismatches': mismatches,
                          'report': str(output / 'comparison.json')}))
        if mismatches:
            raise RuntimeError('integer output mismatch')
    finally:
        for process in roles:
            if process.poll() is None:
                process.terminate()
        for process in roles:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == '__main__':
    main()
