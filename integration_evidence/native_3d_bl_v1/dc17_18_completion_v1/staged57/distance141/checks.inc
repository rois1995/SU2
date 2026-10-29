using Rational=boost::multiprecision::cpp_rational;
size_t checks=0;
void check(TriangleDistanceBounds b,Rational exact) {
  assert(std::isfinite(b.lower) && std::isfinite(b.upper) && b.lower>=0 && b.upper>=b.lower);
  assert(Rational(b.lower)*Rational(b.lower)<=exact);
  assert(Rational(b.upper)*Rational(b.upper)>=exact);
  ++checks;
}
int main(){
  AtlasOps op;
  // Nearest points respectively face, three edges and three vertices of x+y<=4.
  const std::array<Point,7> queries={{{1,1,2},{2,-1,2},{-1,2,2},{3,3,2},{-1,-1,2},{5,-1,2},{-1,5,2}}};
  const int dist2[]={4,5,5,6,6,6,6};
  for(int perm=0;perm<3;++perm)for(int scale: {1,2,8})for(int shift: {-16,0,16}) {
    auto transform=[&](Point p){double a[]={p.x,p.y,p.z};return Point{double(shift+scale*a[perm]),double(shift+scale*a[(perm+1)%3]),double(shift+scale*a[(perm+2)%3])};};
    std::array<Point,3> tri={transform({0,0,0}),transform({4,0,0}),transform({0,4,0})};
    for(size_t i=0;i<queries.size();++i){auto p=transform(queries[i]);auto box=Exact(p);Rational exact=dist2[i]*scale*scale;
      check(OldTriangleDistance(box,tri,op),exact);check(NewFull(box,tri,op),exact);check(NewUpper(box,tri,op),exact);
      // Every corner is a possible query in the same nonzero-width box.
      for(auto& r:box){r.lo-=.125L*scale;r.hi+=.125L*scale;}
      auto old=OldTriangleDistance(box,tri,op),full=NewFull(box,tri,op),upper=NewUpper(box,tri,op);
      for(unsigned mask=0;mask<8;++mask){
        Rational original[3];for(unsigned d=0;d<3;++d)original[(perm+d)%3]=(Rational(mask&(1u<<d)?box[d].hi:box[d].lo)-shift)/scale;
        auto x=original[0],y=original[1],z=original[2];Rational squared=z*z;
        if(i==1)squared+=y*y; if(i==2)squared+=x*x;
        if(i==3)squared+=(x+y-4)*(x+y-4)/2;
        if(i==4)squared+=x*x+y*y; if(i==5)squared+=(x-4)*(x-4)+y*y; if(i==6)squared+=x*x+(y-4)*(y-4);
        squared*=scale*scale;check(old,squared);check(full,squared);check(upper,squared);
      }
    }
  }
  std::array<Point,3> tri={Point{.58904233025368091,.38581769422093798,-.03112997494669438},Point{.56530953543327178,.39977586224200762,-.03225505132526349},Point{.56375529165422267,.36510764045479471,-.032187514520798287}};
  std::array<Box,64> points;for(size_t i=0;i<points.size();++i){double u=(i%8)/8.,v=(i/8)/8.;points[i]=Exact({tri[0].x+(tri[1].x-tri[0].x)*u+(tri[2].x-tri[0].x)*v,tri[0].y+(tri[1].y-tri[0].y)*u+(tri[2].y-tri[0].y)*v,tri[0].z+(tri[1].z-tri[0].z)*u+(tri[2].z-tri[0].z)*v+.00013});}
  volatile double sink=0;auto time=[&](auto f){auto start=std::chrono::steady_clock::now();for(unsigned repeat=0;repeat<400;++repeat)for(const auto& p:points)sink+=f(p,tri,op).upper;return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
  std::cout<<"checks="<<checks<<" old_seconds="<<time(OldTriangleDistance)<<" new_full_seconds="<<time(NewFull)<<" new_upper_seconds="<<time(NewUpper)<<" sink="<<sink<<"\n";
}
