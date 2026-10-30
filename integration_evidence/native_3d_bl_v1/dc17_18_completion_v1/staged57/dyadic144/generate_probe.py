import pathlib,json
p=pathlib.Path(__file__).resolve().parent;old=(p.parent/'surface_scope_probe/probe.cpp').read_text(); data=json.loads((p/'queries.json').read_text());prefix=old[:old.index('    const std::vector<SU2Native3D::Id> facets=')];code=prefix+'    struct Query {std::vector<SU2Native3D::Id> facets;std::vector<std::array<SU2Native3D::Point,3>> physical;};\n    std::vector<Query> queries;\n'
for q in data:
 facets=','.join(map(str,q['facets']));triangles=','.join('{{'+','.join('{'+','.join(format(v,'.17g') for v in point)+'}' for point in tri)+'}}' for tri in q['physical']);code+='    queries.push_back({{'+facets+'},{'+triangles+'}});\n'
code+='''    unsigned failed=0;for(size_t i=0;i<queries.size();++i){
      SU2NativeGeometry::AtlasBudget budget;budget.maximum_work=1000000000;budget.maximum_bytes=ceiling;budget.maximum_integer_bits=209504;budget.caller_bytes=caller;
      const auto start=std::chrono::steady_clock::now();
      try {auto bound=geometry.BoundPhysicalSurface(queries[i].facets,queries[i].physical,1e-4,budget);
        std::cout<<std::setprecision(17)<<"query="<<i<<" result=bound bound="<<bound;
      } catch(const std::exception& e){++failed;std::cout<<"query="<<i<<" result=exception error="<<e.what();}
      std::cout<<" work="<<budget.work<<" caller_bytes="<<caller<<" peak_bytes="<<budget.peak_bytes<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\\n"<<std::flush;
    }
    std::cout<<"completed="<<queries.size()<<" exceptions="<<failed<<"\\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<"\\n";status=1;}
  SU2_MPI::Finalize();return status;
}
''';(p/'probe.cpp').write_text(code)
