import atexit

from ctypes import *

#libredis = cdll.LoadLibrary("Debug/libredis.so")
libredis = cdll.LoadLibrary("Release/libredis.so")

libredis.Module_init()
atexit.register(libredis.Module_free)

class Connection(object):
    def __init__(self, addr):
        self._connection = libredis.Connection_new(addr)

    def get(self, key):
        batch = Batch()
        batch.write("GET %s\r\n", key)
        batch.add_command()
        batch.execute(self)
        reply = batch.next_reply()
        return reply.value
    
    def execute(self, batch):    
        libredis.Connection_execute(self._connection, batch._batch)

    def free(self):
        libredis.Connection_free(self._connection)
        self._connection = None

    def __del__(self):
        if self._connection is not None:
            self.free()

class ConnectionManager(object):
    def __init__(self):
        self._connections = {}
            
    def get_connection(self, addr):
        if not addr in self._connections:
            self._connections[addr] = Connection(addr)
        return self._connections[addr]
        
class Reply(object):
    RT_OK = 1
    RT_ERROR = 2
    RT_BULK_NIL = 3
    RT_BULK = 4
    RT_MULTIBULK_NIL = 5
    RT_MULTIBULK = 6
    
    def __init__(self, reply):
        self._reply = reply

    @property
    def type(self):
        return libredis.Reply_type(self._reply)

    def has_child(self):
        return bool(libredis.Reply_has_child(self._reply))

    def pop_child(self):
        return Reply(libredis.Reply_pop_child(self._reply))
    
    def dump(self):
        libredis.Reply_dump(self._reply)

    def free(self):
        libredis.Reply_free(self._reply)
        self._reply = None

    def __del__(self):
        if self._reply is not None:
            self.free()

    @property
    def value(self):
        if self.type in [self.RT_BULK_NIL, self.RT_MULTIBULK_NIL]:
            return None
        else:
            return string_at(libredis.Reply_data(self._reply), libredis.Reply_length(self._reply))

class Buffer(object):
    def __init__(self, buffer):
        self._buffer = buffer
        
    def dump(self, limit = 64):
        libredis.Buffer_dump(self._buffer, limit)
        
class Batch(object):
    def __init__(self):
        self._batch = libredis.Batch_new()

    def write(self, format, *args):
        libredis.Batch_write(self._batch, format, *args)

    def add_command(self):
        libredis.Batch_add_command(self._batch)
    
    def execute(self, connection):
        libredis.Batch_execute(self._batch, connection._connection)

    def has_reply(self):
        return bool(libredis.Batch_has_reply(self._batch))

    def pop_reply(self):
        return Reply(libredis.Batch_pop_reply(self._batch))

    @property
    def write_buffer(self):
        return Buffer(libredis.Batch_write_buffer(self._batch))
        
    def free(self):
        libredis.Batch_free(self._batch)
        self._batch = None

    def __del__(self):
        if self._batch is not None:
            self.free()

class Ketama(object):
    libredis.Ketama_get_server.restype = c_char_p
    
    def __init__(self):
        self._ketama = libredis.Ketama_new()

    def add_server(self, addr, weight):
        libredis.Ketama_add_server(self._ketama, addr[0], addr[1], weight)

    def create_continuum(self):
        libredis.Ketama_create_continuum(self._ketama)

    def get_server(self, key):
        return libredis.Ketama_get_server(self._ketama, key, len(key))

    def free(self):
        libredis.Ketama_free(self._ketama)
        self._ketama = None

    def __del__(self):
        if self._ketama is not None:
            self.free()
            
class Redis(object):
    def __init__(self, server_hash, connection_manager):
        self.server_hash = server_hash
        self.connection_manager = connection_manager

    def _execute_simple(self, key, format, *args):
        batch = Batch()
        batch.write(format, *args)
        batch.add_command()
        server_addr = self.server_hash.get_server(key)
        connection = self.connection_manager.get_connection(server_addr)
        connection.execute(batch)
        libredis.Module_dispatch()
        return batch.pop_reply()
        
    def set(self, key, value):
        return self._execute_simple(key, "SET %s %d\r\n%s\r\n", key, len(value), value)
        
    def mget(self, *keys):
        batches = {}
        #add all keys to batches
        for key in keys:
            server_ip = self.server_hash.get_server(key)
            batch = batches.get(server_ip, None)
            if batch is None: #new batch
                batch = Batch()
                batch.write("MGET")
                batch.keys = []
                batches[server_ip] = batch
            batch.write(" %s", key)
            batch.keys.append(key)
        #finalize batches, and start executing
        for server_ip, batch in batches.items():
            batch.write("\r\n")
            batch.add_command()
            connection = self.connection_manager.get_connection(server_ip)
            connection.execute(batch)
        #handle events until all complete
        libredis.Module_dispatch()
        #build up results
        results = {}
        for batch in batches.values():
            #only expect 1 (multibulk) reply per batch
            reply = batch.pop_reply()
            for key in batch.keys:
                child = reply.pop_child()
                results[key] = child.value
        return results
    
