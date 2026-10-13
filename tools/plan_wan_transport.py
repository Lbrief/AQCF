#!/usr/bin/env python3
"""Write the retained 12-endpoint/eight-SSH WAN topology; never open a connection."""
import argparse
import json
from pathlib import Path
import shlex


def port_groups(base):
    if not 45000 <= base < 49000:
        raise ValueError('base port must be in [45000,49000)')
    ports = [base + 1000 * lane + offset for lane in range(4) for offset in (0, 50, 100)]
    groups = []
    for index in range(0, 12, 3):
        groups.extend(((ports[index], ports[index + 2]), (ports[index + 1],)))
    assert len(groups) == 8 and sorted(p for group in groups for p in group) == sorted(ports)
    assert max(ports) + 5000 < 65536
    return ports, groups


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', required=True, help='User-configured SSH host alias')
    parser.add_argument('--base-port', type=int, default=47000)
    parser.add_argument('--relay-cpus', required=True, help='Allowed server CPU IDs for transport helpers')
    parser.add_argument('--server-log', required=True, help='Fresh absolute Linux server relay log path')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.server.startswith('-') or any(c.isspace() for c in args.server):
        parser.error('server must be a single SSH host alias')
    cpus = [int(cpu) for cpu in args.relay_cpus.split(',')]
    if not 1 <= len(cpus) <= 4 or len(set(cpus)) != len(cpus) or min(cpus) < 0:
        parser.error('provide one to four distinct nonnegative relay CPU IDs')
    if not args.server_log.startswith('/'):
        parser.error('--server-log must be absolute on the server')
    ports, groups = port_groups(args.base_port)
    commands = []
    for group in groups:
        command = ['ssh', '-N', '-T', '-o', 'ControlMaster=no', '-o', 'ControlPath=none',
                   '-o', 'ExitOnForwardFailure=yes', '-o', 'ServerAliveInterval=15',
                   '-o', 'ServerAliveCountMax=10']
        for port in group:
            command += ['-L', f'127.0.0.1:{port}:127.0.0.1:{port+5000}']
        commands.append(command + [args.server])
    config = {'ports': ports, 'offset': 5000, 'cpus': cpus,
              'deadline_seconds': 7200, 'log': args.server_log}
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'relay.json').write_text(json.dumps(config, indent=2) + '\n')
    (args.output / 'transport.json').write_text(json.dumps({
        'native_ports': ports, 'ssh_groups': groups, 'ssh_commands': commands,
        'native_endpoint_count': 12, 'ssh_connection_count': 8,
        'execution_performed': False, 'management_transport_included': False,
        'scope': 'Transport plan only; role deployment and readiness control must be supplied as described in docs/wan.md',
    }, indent=2) + '\n')
    (args.output / 'ssh_commands.txt').write_text('\n'.join(shlex.join(c) for c in commands) + '\n')
    print(f'WAN_TRANSPORT_PLAN_ONLY: {args.output}; no SSH, socket, or inference executed')


if __name__ == '__main__':
    main()
