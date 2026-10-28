set pagination off
set confirm off
set print elements 40
break CNativeGeometryBackend3D.cpp:136
commands
silent
printf "FIRST_NONCONVERGED_MODEL_CANDIDATE\n"
print id
print u
print v
print query
print scope.hint
print best
print net
bt 7
quit
end
run
