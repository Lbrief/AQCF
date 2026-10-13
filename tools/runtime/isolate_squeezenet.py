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
    if a.probe_only:
        argv=loader+["--list","/program/aqcf_parallel_portable"]
    else:
        import hashlib
        release=json.loads(Path("/release.json").read_text())
        env["AQCF_CPU_LIST"]=",".join(map(str,release["cpus"]))
        assert release["real"] is True and release["candidate427"]==1
        env["AQCF_SQ266"]="1"
        env["AQCF_POOL273"]="1"
        env["AQCF_TREE280"]="1"
        env["AQCF_SQ_WIRE288"]="1"
        env["AQCF_RETAINED303"]="1"
        env["AQCF_JOINT302"]="1"
        env["AQCF_COMMON317"]="1"
        env["AQCF_SQ_INPUT379"]="1"
        env["AQCF_SUPPORT402"]="1"
        env["AQCF_PACK408"]="1"
        env["AQCF_PIPE416"]="1"
        env["AQCF_POOLMUX426"]=str(release["candidate427"])
        env["AQCF_KEY_SLICE207"]="1"
        env["AQCF_WORD_SLICE208"]="1"
        env["AQCF_RADIX197"]="0"
        env["AQCF_CALLBACK_ALLOW"]="/program/ALLOW.txt"
        env["AQCF_SOURCE_BOUND_PRIVATE_GATE"]="QUALIFIED_SQ426_COMPLETE_MODEL"
        proof=hashlib.sha256(Path("/assets/OWN.txt").read_bytes()).hexdigest()
        argv=loader+["/program/aqcf_parallel_portable",str(a.role),str(a.port),str(a.workers),
              "/program/CLIENT_LAYOUT.bin","/assets/OWN.bin","/work/RESULTS",
              "REAL" if release["real"] else "PUBLIC",proof]
    os.execve(argv[0], argv, env)


if __name__ == "__main__":
    main()
