"""Fresh cloud public/real qualifications with original hybrid complete graph."""
import hashlib,json,os,selectors,signal,struct,subprocess,sys,time,traceback
from pathlib import Path
R=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def put(p,x):
    with Path(p).open('x') as f:json.dump(x,f,indent=2);f.write('\n');f.flush();os.fsync(f.fileno())
def clock():return time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW)
def inventory(jobs):
    rows=[]
    for job in jobs:
        pending=[job.pid];seen=set()
        while pending:
            pid=pending.pop()
            if pid in seen:continue
            seen.add(pid)
            try:
                st=Path('/proc')/str(pid)/'status'
                data=dict(x.split(':',1) for x in st.read_text().splitlines() if ':' in x)
                tasks=[]
                for task in (st.parent/'task').iterdir():
                    td=dict(x.split(':',1) for x in (task/'status').read_text().splitlines() if ':' in x)
                    fields=(task/'stat').read_text().rsplit(')',1)[1].split()
                    tasks.append({'tid':int(task.name),'cpus':td['Cpus_allowed_list'].strip(),'namespace_tids':list(map(int,td['NSpid'].split())),'birth':int(fields[19]),'ticks':int(fields[11])+int(fields[12]),'affinity':sorted(os.sched_getaffinity(int(task.name)))})
                    cp=task/'children'
                    if cp.exists():pending.extend(map(int,cp.read_text().split()))
                rows.append({'pid':pid,'name':data['Name'].strip(),'rss':data.get('VmRSS','').strip(),
                  'hwm':data.get('VmHWM','').strip(),'threads':data['Threads'].strip(),'uid':int(data['Uid'].split()[0]),'tasks':tasks})
            except (FileNotFoundError,ProcessLookupError):pass
    return rows
def pair(commands,dest):
    jobs=[];handles=[];sel=selectors.DefaultSelector();buffers=[b'',b''];assets=set();ready=set();connected=False;go=False
    begin=clock();mono=time.monotonic();last=-1;events=[];last_progress=mono;node_start=[None,None]
    try:
        for i,cmd in enumerate(commands):
            err=(dest/f'role{i+1}.stderr').open('xb');out=(dest/f'role{i+1}.stdout').open('xb');handles.extend([err,out])
            p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,start_new_session=True)
            jobs.append(p);sel.register(p.stdout,selectors.EVENT_READ,i)
        with (dest/'RESOURCES.jsonl').open('x') as rf:
            while any(p.poll() is None for p in jobs) or sel.get_map():
                elapsed=time.monotonic()-mono
                if elapsed>1800:raise TimeoutError('1800s R50 public qualification ceiling; preserve failure')
                if go and time.monotonic()-last_progress>240:raise TimeoutError('No R50 node or online preparation progress for 240s')
                if go and any(t is not None and time.monotonic()-t>900 for t in node_start):raise TimeoutError('R50 node including complete online preparation exceeded900s')
                if int(elapsed)>last:
                    last=int(elapsed);rf.write(json.dumps({'elapsed':elapsed,'loadavg':os.getloadavg(),'processes':inventory(jobs)})+'\n');rf.flush()
                for key,_ in sel.select(.05):
                    i=key.data;chunk=os.read(key.fd,65536)
                    if not chunk:sel.unregister(key.fileobj);continue
                    handles[2*i+1].write(chunk);handles[2*i+1].flush();buffers[i]+=chunk
                    while b'\n' in buffers[i]:
                        line,buffers[i]=buffers[i].split(b'\n',1)
                        if line.startswith((b'NODE ',b'NODE_BEGIN ',b'POOL_STAGE ',b'LINEAR18_PREPARE ',b'BN19_INIT ')):
                            last_progress=time.monotonic()
                            if line.startswith(b'NODE_BEGIN '):node_start[i]=last_progress
                            elif line.startswith(b'NODE '):node_start[i]=None
                        if i==1 and line.startswith(b'CLOCK_BOUNDARY037 '):
                            print(line.decode()+' '+str(clock())+' '+str(time.monotonic_ns()),flush=True)
                        if line==b'ASSETS_READY':assets.add(i)
                        if line==b'READY':ready.add(i)
                        if line.startswith(b'NODE ') or line in (b'FINAL100',b'DONE',b'CLIENT_FINAL100_VERIFIED'):
                            print(dest.name,'ROLE',i+1,line.decode(),flush=True)
                if len(assets)==2 and not connected:
                    for p in jobs:p.stdin.write(b'C');p.stdin.flush()
                    connected=True;events.append({'event':'both_assets_ready_connect','raw_elapsed':(clock()-begin)/1e9})
                if len(ready)==2 and not go:
                    for p in jobs:p.stdin.write(b'G');p.stdin.flush()
                    go=True;last_progress=time.monotonic();events.append({'event':'both_protocol_ready_go','raw_elapsed':(clock()-begin)/1e9})
                if any(p.poll() not in (None,0) for p in jobs):raise RuntimeError('party failure, preserve all outputs and stop')
        assert all(p.returncode==0 for p in jobs)
        return {'complete_process_raw_seconds':(clock()-begin)/1e9,'complete_process_monotonic_seconds':time.monotonic()-mono,
                'exit_codes':[p.returncode for p in jobs],'events':events}
    finally:
        for p in jobs:
            if p.poll() is None:
                try:os.killpg(p.pid,signal.SIGKILL)
                except ProcessLookupError:pass
            p.wait()
        for f in handles:f.close()
        sel.close()