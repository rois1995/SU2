set pagination off
set confirm off
handle SIGPIPE nostop noprint pass
break CBaseMPIWrapper::Error
run
print ErrorMsg
print FunctionName
bt 8
quit
