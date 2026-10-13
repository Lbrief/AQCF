"""Opaque bounded relay with durable logs, independent of management stdin/stdout.

Engineering lifecycle repair only. No protocol parsing, reconnection, payload
replay, changes to crypto/model, or claims about WAN speed/security proof.
All data listeners and targets are loopback. Real deployment must retain SSH.
"""
import json,os,signal,socket,sys,threading,time
from pathlib import Path

class Relay:
    def __init__(self,ports,offset,cpus,deadline,log_path):
        assert ports and len(set(ports))==len(ports)
        assert all(1024<=p<65536 and 1024<=p+offset<65536 for p in ports)
        assert 0<deadline<=7300 and 1<=len(cpus)<=4
        os.sched_setaffinity(0,set(cpus))
        self.ports=ports;self.offset=offset;self.deadline=deadline
        self.stop=threading.Event();self.lock=threading.Lock()
        self.listeners=[];self.connections=[];self.workers=[]
        self.start=time.monotonic();self.reason='unstarted'
        self.stats={'c2s':0,'s2c':0,'connections':0,'per_port':{str(p):{'c2s':0,'s2c':0} for p in ports},'errors':[]}
        self.log=Path(log_path).open('x',buffering=1)
        os.chmod(log_path,0o600)
    def emit(self,marker,extra=None):
        with self.lock:
            self.log.write(marker+' '+json.dumps({**self.stats,'time':time.time(),'elapsed_raw_ns':time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW),'elapsed_seconds':time.monotonic()-self.start,**(extra or {})})+'\n')
            self.log.flush()
    def error(self,label,error):
        with self.lock:self.stats['errors'].append(label+': '+str(error))
    def spawn(self,fn,*args):
        t=threading.Thread(target=fn,args=args,daemon=True)
        with self.lock:self.workers.append(t)
        t.start();return t
    @staticmethod
    def options(s):
        s.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
        s.setsockopt(socket.SOL_SOCKET,socket.SO_SNDBUF,262144)
        s.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,262144)
    def pipe(self,a,b,direction,port):
        try:
            while not self.stop.is_set():
                payload=a.recv(65536)
                if not payload:break
                b.sendall(payload)
                with self.lock:
                    self.stats[direction]+=len(payload)
                    self.stats['per_port'][str(port)][direction]+=len(payload)
        except OSError as error:
            if not self.stop.is_set():self.error(direction,error)
        finally:
            try:b.shutdown(socket.SHUT_WR)
            except OSError:pass
    def handle(self,client,port):
        server=None
        try:
            self.options(client);begin=time.monotonic()
            while not self.stop.is_set():
                try:server=socket.create_connection(('127.0.0.1',port),timeout=1);break
                except OSError:
                    if time.monotonic()-begin>120:raise
                    self.stop.wait(.02)
            if server is None:return
            server.settimeout(None);self.options(server)
            with self.lock:
                self.connections.extend((client,server));self.stats['connections']+=1
            back=self.spawn(self.pipe,server,client,'s2c',port)
            self.pipe(client,server,'c2s',port)
            back.join(timeout=max(0,self.deadline-(time.monotonic()-self.start)))
        except OSError as error:
            if not self.stop.is_set():self.error('connect',error)
        finally:
            client.close()
            if server:server.close()
    def accept(self,listener,port):
        while not self.stop.is_set():
            try:client,_=listener.accept()
            except OSError:return
            self.spawn(self.handle,client,port)
    def request_stop(self,signum,frame):
        self.reason='signal_'+str(signum);self.stop.set()
    def run(self):
        try:
            for port in self.ports:
                listener=socket.socket();listener.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
                listener.bind(('127.0.0.1',port+self.offset));listener.listen(4)
                self.listeners.append(listener);self.spawn(self.accept,listener,port)
            self.reason='running'
            self.emit('TUNNEL_READY',{'cpus':sorted(os.sched_getaffinity(0)),'ports':self.ports,'channels':len(self.ports),'management_pipe_not_required':True})
            last=0
            while not self.stop.wait(.1):
                now=time.monotonic()
                if now-self.start>=self.deadline:
                    self.reason='transport_deadline';self.error('deadline','bounded relay deadline reached');break
                if now-last>=5:self.emit('TRANSPORT_PROGRESS');last=now
        finally:
            self.stop.set()
            with self.lock:sockets=list(self.listeners+self.connections)
            for s in sockets:
                try:s.shutdown(socket.SHUT_RDWR)
                except OSError:pass
                s.close()
            until=time.monotonic()+3
            while any(t.is_alive() for t in self.workers) and time.monotonic()<until:time.sleep(.02)
            self.emit('TRANSPORT_FINAL',{'stop_reason':self.reason,'threads_remaining':sum(t.is_alive() for t in self.workers),'relay_process_CPU_s':time.process_time()})
            os.fsync(self.log.fileno());self.log.close()
        return 0 if self.reason.startswith('signal_') else 2

def main():
    config=json.loads(sys.argv[1])
    relay=Relay(config['ports'],config['offset'],config['cpus'],config['deadline_seconds'],config['log'])
    for sig in (signal.SIGTERM,signal.SIGINT):signal.signal(sig,relay.request_stop)
    return relay.run()
if __name__=='__main__':raise SystemExit(main())
