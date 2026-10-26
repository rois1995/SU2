set pagination off
set print elements 200
python
active = False
class Begin(gdb.Breakpoint):
    def stop(self):
        global active
        active = True
        return False
class Result(gdb.FinishBreakpoint):
    def stop(self):
        print("GEOMETRY_EXCESS_RETURN", self.return_value)
        return False
class Geometry(gdb.Breakpoint):
    def stop(self):
        if active:
            Result(gdb.newest_frame(), internal=True)
        return False
Begin("Common/src/adaptation/CNativeEngine3D.cpp:3528", internal=True)
Geometry("SU2Native3D::(anonymous namespace)::GeometryExcess", internal=True)
end
break Common/src/adaptation/CNativeEngine3D.cpp:3593
commands
silent
print reason
continue
end
run
