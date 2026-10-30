set pagination off
set confirm off
set print elements 0
set print repeats 0
set print pretty on
break ../../Common/src/adaptation/CNativeAffineReference.cpp:405
commands
silent
bt
python
f=gdb.newest_frame()
while f:
 name=f.name() or ''
 if 'UnionChildren' in name or 'QuerySurfaceUnion' in name or 'operator()' in name:
  f.select()
  print('CAPTURE_FRAME',name)
  for key in ['parent','domain','node','upper','lower','initial_upper','from_model','tolerance','facets','physical','include_model','include_mesh','model_pieces']:
   try: print(key, '=', f.read_var(key))
   except Exception: pass
 f=f.older()
end
detach
quit
end
continue
