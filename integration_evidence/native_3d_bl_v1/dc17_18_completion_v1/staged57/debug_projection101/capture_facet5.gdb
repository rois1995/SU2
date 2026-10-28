set pagination off
set confirm off
set print elements 40
break CNativeGeometryBackend3D.cpp:136 if id == 5
commands
silent
printf "NONCONVERGED_FACET5\n"
python
for depth in range(5):
    gdb.write("FRAME %d\n" % depth)
    for expression in ["id", "u", "v", "query", "scope", "best", "net", "*__closure"]:
        try: gdb.write(expression+" "+gdb.execute("print "+expression,to_string=True))
        except gdb.error as e: gdb.write(expression+" unavailable: "+str(e)+"\n")
    try: gdb.execute("up",to_string=True)
    except gdb.error: break
end
bt 8
quit
end
run
