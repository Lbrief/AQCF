"""Trusted launcher: mount/PID namespace, minimal role chroot, then unprivileged role.

The controller invokes this through unshare --mount --pid --fork. It deliberately
does not unshare network: both parties share the enclosing PrivateNetwork unit.
No project root, host /proc, host /tmp, opponent asset, or common admission is
mounted in the role. This is local execution isolation, not a new proof of SCI.
"""
import argparse
import ctypes
import json
import os
import resource
import subprocess
from pathlib import Path


def mount(*argv):
    subprocess.run(["/usr/bin/mount", *map(str, argv)], check=True,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)


def bind(root, source, destination, readonly=True):
    target = root / destination.lstrip("/")
    source = Path(source).resolve(strict=True)
    if source.is_dir():
        target.mkdir(parents=True, exist_ok=True)
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.touch(exist_ok=False)
    mount("--bind", source, target)
    if readonly:
        mount("-o", "remount,bind,ro,nosuid", target)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--program", type=Path, required=True)
    p.add_argument("--runtime", type=Path, required=True)
    p.add_argument("--asset", type=Path, required=True)
    p.add_argument("--release", type=Path, required=True)
    p.add_argument("--proof", type=Path, required=True)
    p.add_argument("--role", type=int, choices=[1, 2], required=True)
    p.add_argument("--port", type=int, required=True)
    p.add_argument("--workers", type=int, choices=[1,4], required=True)
    p.add_argument("--raw-sha256")
    p.add_argument("--packet-sha256")
    p.add_argument("--probe-only", action="store_true")
    a = p.parse_args()
    root = a.root.resolve(strict=True)
    if os.geteuid() != 0:
        raise RuntimeError("Namespace setup requires the parent root-owned service")
    # This is a fresh role jail, never a shared/root workspace. The service's
    # restrictive umask must not make loader/asset mountpoints untraversable
    # after the UID drop. Private contents remain single-owner mount bindings.
    os.umask(0o022)
    os.chmod(root,0o755)
    # Never mutate propagation of the host mount namespace.
    if os.readlink("/proc/self/ns/mnt") == os.readlink("/proc/1/ns/mnt"):
        raise RuntimeError("Missing isolated mount namespace")
    mount("--make-rprivate", "/")
    bind(root, "/usr", "/usr")
    for name in ("lib", "lib64", "bin", "sbin"):
        source = Path("/") / name
        target = root / name
        if source.is_symlink():
            target.symlink_to(os.readlink(source))
        elif source.exists():
            bind(root, source, "/" + name)
    for name in ("ld.so.cache", "localtime"):
        if (Path("/etc") / name).exists():
            bind(root, Path("/etc") / name, "/etc/" + name)
    for name in ("null", "zero", "urandom", "random"):
        bind(root, Path("/dev") / name, "/dev/" + name, readonly=False)
    bind(root, a.program, "/program")
    # Existing portable shared libraries, no input/model assets in this folder.
    bind(root, a.runtime, "/runtime")
    bind(root, "/sys/devices/system/cpu", "/sys/devices/system/cpu")
    asset_name = "OWN.bin"
    bind(root, a.asset, "/assets/" + asset_name)
    bind(root, a.release, "/release.json")
    bind(root, a.proof, "/assets/OWN.txt")
    bind(root, a.work, "/work", readonly=False)
    uid = 43001 if a.role == 1 else 43002
    # chmod/chown target is exclusively the fresh owned work directory.
    os.chown(a.work, uid, uid)
    os.chmod(a.work, 0o700)
    (root / "tmp").mkdir()
    mount("-t", "tmpfs", "-o", "mode=1777,size=64m,nosuid,nodev", "tmpfs", root / "tmp")
    (root / "proc").mkdir()
    mount("-t", "proc", "-o", "nosuid,nodev,noexec", "proc", root / "proc")
    os.chroot(root)
    os.chdir("/work")
    os.setgroups([])
    os.setgid(uid)
    os.setuid(uid)
    # Per-role, per-session empty OT cache; never reuse earlier setup.
    Path("/work/data").mkdir(mode=0o700, exist_ok=False)
    if list(Path("/work/data").iterdir()):
        raise RuntimeError("OT work directory was not fresh")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    if ctypes.CDLL(None, use_errno=True).prctl(38, 1, 0, 0, 0) != 0:
        raise RuntimeError("Cannot enable no_new_privs")
    # Inherited descriptors are the control input pipe and this role's logs.
    visible = sorted(x.name for x in Path("/assets").iterdir())
    if visible != ["OWN.bin", "OWN.txt"] or Path("/mnt/g").exists() or Path("/root").exists():
        raise RuntimeError("Role filesystem closure failed")
    if len(list(Path("/proc").glob("[0-9]*"))) > 3:
        raise RuntimeError("Host/opponent processes visible in role PID namespace")
    # Verify the actual dropped UID can write its own DrvFS-backed directory.
    # chown/chmod succeeding on the host is not sufficient evidence of this.
    public_text = "PUBLIC_ROLE_OWN_WORK_WRITE_READ_CHECK\n"
    with Path("/work/PUBLIC_WRITABLE_PROBE.txt").open("x") as f:
        f.write(public_text)
    if Path("/work/PUBLIC_WRITABLE_PROBE.txt").read_text() != public_text:
        raise RuntimeError("Role own work directory write/read check failed")
    receipt = dict(role=a.role, uid=os.getuid(), assets=visible,
                   mount_namespace=os.readlink("/proc/self/ns/mnt"),
                   pid_namespace=os.readlink("/proc/self/ns/pid"),
                   network_namespace=os.readlink("/proc/self/ns/net"),
                   host_project_visible=False, no_new_privs=True,
                   own_work_write_read_verified=True)
    print("AQCF_PARALLEL_ROLE_ISOLATION " + json.dumps(receipt), flush=True)
    env = dict(PATH="/usr/bin:/bin", HOME="/work", TMPDIR="/tmp", LANG="C.UTF-8",
               OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
               CUDA_VISIBLE_DEVICES="", LD_LIBRARY_PATH="/runtime", PCN_HE_WORKERS="4", AQCF_COMPUTE4_CLOUD_PUBLIC="1")
    loader=["/runtime/ld-linux-x86-64.so.2","--library-path","/runtime"]
    assert all(os.access(p,os.R_OK) for p in ('/assets/OWN.bin','/program/CLIENT_LAYOUT.bin','/program/ALLOW.txt')), 'dropped UID asset read-access preflight'
    if a.probe_only:
        argv=loader+["--list","/program/aqcf_parallel_portable"]
    else:
        import hashlib
        release=json.loads(Path("/release.json").read_text())
        env["AQCF_CPU_LIST"]=",".join(map(str,release["cpus"]))
        assert release["quotient169"]in(0,1)
        env["AQCF_QUOTIENT169"]=str(release["quotient169"])
        assert release["bn191"] in (0,1) and (not release["bn191"] or release["quotient169"])
        env["AQCF_BN191"]=str(release["bn191"])
        assert release["radix197"] in (0,1) and release["bn191"]==release["quotient169"]==1
        env["AQCF_RADIX197"]=str(release["radix197"])
        assert release["radix197"]==0 and release["slices209"] in (0,1)
        env["AQCF_KEY_SLICE207"]=str(release["slices209"])
        env["AQCF_WORD_SLICE208"]=str(release["slices209"])
        assert release["slices209"]==1 and release["input213"] in (0,1)
        env["AQCF_DOT_INPUT213"]=str(release["input213"])
        assert release["input213"]==1 and release["floor216"] in (0,1)
        env["AQCF_FLOOR216"]=str(release["floor216"])
        assert release["floor216"]==1 and release["seed221"] in (0,1)
        env["AQCF_SEED221"]=str(release["seed221"])
        assert release["seed221"]==1 and release["frontier233"] in (0,1)
        env["AQCF_FRONTIER233"]=str(release["frontier233"])
        assert release["frontier233"]==1 and release["mixed238"] in (0,1)
        env["AQCF_MIXED238"]=str(release["mixed238"])
        assert release["mixed238"]==1 and release["elide250"] in (0,1)
        env["AQCF_ELIDE250"]=str(release["elide250"])
        assert release["elide250"]==1 and release["pool258"] in (0,1)
        env["AQCF_POOL258"]=str(release["pool258"])
        assert release["pool258"]==1 and release["horner293"] in (0,1)
        env["AQCF_HORNER292"]=str(release["horner293"])
        assert release["horner293"]==1 and release["pool295"] in (0,1)
        env["AQCF_POOL295"]=str(release["pool295"])
        assert release["pool295"]==1 and release["retained299"] in (0,1)
        env["AQCF_RETAINED299"]=str(release["retained299"])
        env["AQCF_JOINT304"]=str(release["joint305"])
        env["AQCF_RETAINED307"]=str(release["retained307"])
        env["AQCF_COMMON315"]=str(release["common371"])
        env["AQCF_DUPLEX336"]=str(release["duplex371"])
        env["AQCF_CORRECTION341"]=str(release["correction371"])
        env["AQCF_SEEDED343"]=str(release["seeded371"])
        env["AQCF_PACKED353"]=str(release["packed371"])
        env["AQCF_DIRECTION359"]=str(release["direction371"])
        env["AQCF_COHORT364"]=str(release["direction371"])
        env["AQCF_INPUT371"]=str(release["input371"])
        env["AQCF_POOLFLOOR444"]=str(release["pool444"])
        env["AQCF_POLYPHASE458"]=str(release["poly458"])
        assert release["pipe467"]==1
        env["AQCF_PIPE467"]=str(release["pipe467"])
        assert release["dot500"]==1
        env["AQCF_DOT500"]=str(release["dot500"])
        assert release["merge515"]==1
        env["AQCF_MERGE515"]=str(release["merge515"])
        assert release["real"] is True and release["poly458"]==release["pool444"]==release["input371"]==release["direction371"]==release["packed371"]==release["seeded371"]==release["correction371"]==release["duplex371"]==release["common371"]==release["retained307"]==release["joint305"]==release["retained299"]==1 and release["execution_profile170"] in ("CLOUD_PUBLIC", "WAN_NATIVE")
        env["AQCF_EXECUTION_PROFILE170"]=release["execution_profile170"]
        env["AQCF_CALLBACK_ALLOW"]="/program/ALLOW.txt"
        env["AQCF_SOURCE_BOUND_PRIVATE_GATE"]="QUALIFIED_R50_515_COMPLETE_MODEL"
        proof=hashlib.sha256(Path("/assets/OWN.txt").read_bytes()).hexdigest()
        argv=loader+["/program/aqcf_parallel_portable",str(a.role),str(a.port),str(a.workers),
              "/program/CLIENT_LAYOUT.bin","/assets/OWN.bin","/work/RESULTS",
              "REAL" if release["real"] else "PUBLIC",proof]
    os.execve(argv[0], argv, env)


if __name__ == "__main__":
    main()
