# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _bossao

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types



bossao_new = _bossao.bossao_new

bossao_start = _bossao.bossao_start

bossao_seek = _bossao.bossao_seek

bossao_shutdown = _bossao.bossao_shutdown

bossao_stop = _bossao.bossao_stop

bossao_play = _bossao.bossao_play

bossao_pause = _bossao.bossao_pause

bossao_unpause = _bossao.bossao_unpause

bossao_finished = _bossao.bossao_finished

bossao_time_total = _bossao.bossao_time_total

bossao_time_current = _bossao.bossao_time_current

bossao_filename = _bossao.bossao_filename

bossao_driver_name = _bossao.bossao_driver_name

