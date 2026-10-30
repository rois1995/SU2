set pagination off
handle SIGCONT nostop noprint pass
python
f=gdb.newest_frame()
while f:
 n=f.name() or chr(32)
 if "MappedFacetReference::QuerySurfaceUnion" in n:
  f.select()
  print("query_frame",n)
  try:
   c=f.read_var("this").dereference()["curved_search"]["_M_ptr"].dereference()
   for k in ["prepared_bytes","prepared_limit"]: print(k, str(c[k]))
   for k in ["prepared","scopes"]: print(k+"_entries", str(c[k]["_M_t"]["_M_impl"]["_M_node_count"]))
   for k in ["include_model","include_mesh","model_pieces"]:
    try: print(k,str(f.read_var(k)))
    except Exception as e: print(str(e))
   break
  except Exception as e: print(str(e))
 f=f.older()
end
bt 5
detach
quit
