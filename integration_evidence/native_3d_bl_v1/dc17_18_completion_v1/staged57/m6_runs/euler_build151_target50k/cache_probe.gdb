set pagination off
handle SIGCONT nostop noprint pass
python
f=gdb.newest_frame()
while f:
 if (f.name() or chr(32)).startswith("SU2Native3D::(anonymous namespace)::Adapt"):
  f.select()
  try:
   c=f.read_var("geometry")["parameter_view"]["_M_ptr"].dereference()["accepted"]["_M_ptr"].dereference()["geometry"]["_M_ptr"].dereference()["curved_search"]["_M_ptr"].dereference()
   for k in ["prepared_bytes","prepared_limit"]: print(k, str(c[k]))
   for k in ["prepared","scopes"]: print(k+"_entries", str(c[k]["_M_t"]["_M_impl"]["_M_node_count"]))
  except Exception as e: print(str(e))
  break
 f=f.older()
end
detach
quit
