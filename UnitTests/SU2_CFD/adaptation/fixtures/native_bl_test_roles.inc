// DC-15 TEST-ONLY: crease roles of the fixture loaders from their marker names ("cut" and any name containing "sym" are
// continuations), the former rule of dc9::DetectCreases. Production takes the roles from the configuration marker kinds.
namespace dc9 {
inline std::map<int,int> NameRoles(const State& s) {
  std::map<int,int> roles;
  for(size_t m=0;m<s.markers.size();++m){std::string n=s.markers[m];for(auto& c:n)c=char(std::tolower(c));
    roles[int(m)]=s.walls.count(int(m))?1:(n=="cut"||n.find("sym")!=std::string::npos)?0:2;}
  return roles;
}
inline void DetectCreases(const State& s){DetectCreases(s,NameRoles(s));}
}  // namespace dc9
