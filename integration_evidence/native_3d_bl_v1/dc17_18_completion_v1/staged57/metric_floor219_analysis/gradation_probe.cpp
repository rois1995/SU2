// Isolated diagnostic: original SU2 eigensolver, serial transcription of native gradation.
#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;
using su2double = double;
#include "/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/include/linear_algebra/blas_structure.hpp"
using Matrix=array<array<double,3>,3>;
using Vector=array<double,3>;
using Clock=chrono::steady_clock;

void eig(const Matrix& m,Matrix& q,Vector& v) {
  Vector work{};CBlasStructure::EigenDecomposition(m,q,v,3,work);
}
Matrix compose(const Matrix& q,const Vector& v) {
  Matrix m{};CBlasStructure::EigenRecomposition(m,q,v,3);return m;
}
Matrix multiply(const Matrix& a,const Matrix& b) {
  Matrix c{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k)c[i][j]+=a[i][k]*b[k][j];return c;
}
Matrix whiten(const Matrix& a,const Matrix& r) {
  Matrix c{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)
    for(int k=0;k<3;++k)for(int l=0;l<3;++l)c[i][j]+=r[k][i]*a[k][l]*r[l][j];return c;
}
Matrix intersect(const Matrix& a,const Matrix& b) {
  double scale=0;for(int i=0;i<3;++i)for(int j=0;j<3;++j)scale=max(scale,max(abs(a[i][j]),abs(b[i][j])));
  if(!(scale>0)||!isfinite(scale))throw runtime_error("Bad intersection scale");
  Matrix sum{},na{};for(int i=0;i<3;++i)for(int j=0;j<3;++j){na[i][j]=a[i][j]/scale;sum[i][j]=na[i][j]+b[i][j]/scale;}
  Matrix q{};Vector v{};eig(sum,q,v);Vector sv{},iv{};
  for(int i=0;i<3;++i){if(!(v[i]>0))throw runtime_error("Singular intersection");sv[i]=sqrt(v[i]);iv[i]=1/sv[i];}
  auto s=compose(q,sv), inv=compose(q,iv);auto t=multiply(multiply(inv,na),inv);
  eig(t,q,v);for(auto& x:v)x=max(x,1-x);t=compose(q,v);
  auto raw=multiply(multiply(s,t),s);Matrix result{};
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)result[i][j]=scale*(.5*raw[i][j]+.5*raw[j][i]);return result;
}
void bound(Matrix& m) {
  Matrix q{};Vector v{};eig(m,q,v);for(auto& x:v)x=min(max(x,1e-4),1e12);
  double high=*max_element(v.begin(),v.end());for(auto& x:v)x=max(x,high/1e12);m=compose(q,v);
}
template<class T>void read(ifstream& f,vector<T>& v,size_t n) {
  v.resize(n);f.read(reinterpret_cast<char*>(v.data()),sizeof(T)*n);if(!f)throw runtime_error("Truncated input");
}
int main(int argc,char** argv) {
 try {
  if(argc!=5)throw runtime_error("input.bin output-prefix scale seconds");
  const auto start=Clock::now();double seconds=stod(argv[4]),factor=stod(argv[3]);
  ifstream in(argv[1],ios::binary);uint64_t n=0,arcs=0;in.read(reinterpret_cast<char*>(&n),8);in.read(reinterpret_cast<char*>(&arcs),8);
  if(!in||!n||n>1000000||arcs>100000000)throw runtime_error("Bad input shape");
  vector<Vector> p;vector<double> volume;vector<uint64_t> offsets,neighbors;vector<Matrix> metric;
  read(in,p,n);read(in,volume,n);read(in,offsets,n+1);read(in,neighbors,arcs);read(in,metric,n);
  if(offsets[0]||offsets.back()!=arcs)throw runtime_error("Invalid offsets");
  for(auto j:neighbors)if(j>=n)throw runtime_error("Invalid neighbor");
  for(auto& m:metric){for(auto& row:m)for(auto& x:row)x*=factor;bound(m);}
  vector<Matrix> old(n),previous(n),inverse(n);vector<bool> changed(n,false),sourceChanged(n,false);
  const double growth=log(1.3);uint64_t updates=0,visits=0;unsigned sweep=0;bool converged=false,budget=false;
  for(;sweep<256;++sweep) {
    if(chrono::duration<double>(Clock::now()-start).count()>seconds){budget=true;break;}
    for(uint64_t i=0;i<n;++i)sourceChanged[i]=sweep==0||metric[i]!=previous[i];
    previous=metric;uint64_t roundUpdates=0;
    for(uint64_t i=0;i<n;++i) {
      bool active=changed[i]||sourceChanged[i];for(auto k=offsets[i];k<offsets[i+1]&&!active;++k)active=sourceChanged[neighbors[k]];
      changed[i]=false;if(!active)continue;++visits;
      Matrix q{};Vector v{};eig(previous[i],q,v);
      for(int a=0;a<3;++a){if(!(v[a]>0))throw runtime_error("Non-SPD gradation input");for(int b=0;b<3;++b)inverse[i][b][a]=q[b][a]/sqrt(v[a]);}
      for(auto k=offsets[i];k<offsets[i+1];++k) {
        const auto j=neighbors[k];Vector dx{};double distance2=0;for(int a=0;a<3;++a)dx[a]=p[i][a]-p[j][a];
        for(int a=0;a<3;++a)for(int b=0;b<3;++b)distance2+=dx[a]*previous[j][a][b]*dx[b];
        const double scale=1/pow(1+growth*sqrt(max(0.,distance2)),2);Matrix transported{};
        for(int a=0;a<3;++a)for(int b=0;b<3;++b)transported[a][b]=scale*previous[j][a][b];
        auto t=whiten(transported,inverse[i]);eig(t,q,v);
        if(*max_element(v.begin(),v.end())<=1+1e-7)continue;
        changed[i]=true;metric[i]=intersect(metric[i],transported);
      }
      if(changed[i]){++roundUpdates;++updates;bound(metric[i]);}
    }
    if((sweep+1)%25==0)cerr<<"sweep "<<sweep+1<<" updates "<<roundUpdates<<" seconds "<<chrono::duration<double>(Clock::now()-start).count()<<endl;
    if(!roundUpdates){converged=true;++sweep;break;}
  }
  double complexity=0,maxRatio=1;uint64_t bad=0;
  for(uint64_t i=0;i<n;++i) {
    Matrix q{};Vector v{};eig(metric[i],q,v);complexity+=volume[i]*sqrt(v[0]*v[1]*v[2]);
    for(int a=0;a<3;++a)for(int b=0;b<3;++b)inverse[i][b][a]=q[b][a]/sqrt(v[a]);
    for(auto k=offsets[i];k<offsets[i+1];++k) {
      const auto j=neighbors[k];Vector dx{};double d2=0;for(int a=0;a<3;++a)dx[a]=p[i][a]-p[j][a];
      for(int a=0;a<3;++a)for(int b=0;b<3;++b)d2+=dx[a]*metric[j][a][b]*dx[b];
      double scale=1/pow(1+growth*sqrt(max(0.,d2)),2);Matrix transported=metric[j];for(auto& row:transported)for(auto& x:row)x*=scale;
      auto t=whiten(transported,inverse[i]);eig(t,q,v);double ratio=*max_element(v.begin(),v.end());maxRatio=max(maxRatio,ratio);bad+=ratio>1+1e-5;
    }
  }
  string prefix=argv[2];ofstream out(prefix+".bin",ios::binary);out.write(reinterpret_cast<const char*>(metric.data()),n*sizeof(Matrix));out.close();
  ofstream report(prefix+".json");report<<setprecision(17)<<"{\"points\":"<<n<<",\"arcs\":"<<arcs<<",\"scale\":"<<factor<<",\"sweeps\":"<<sweep
    <<",\"converged\":"<<(converged?"true":"false")<<",\"time_budget\":"<<(budget?"true":"false")<<",\"complexity\":"<<complexity<<",\"max_ratio\":"<<maxRatio<<",\"bad_directed_edges\":"<<bad
    <<",\"updates\":"<<updates<<",\"visits\":"<<visits<<",\"seconds\":"<<chrono::duration<double>(Clock::now()-start).count()<<"}\n";
  cout<<"complexity "<<setprecision(17)<<complexity<<" converged "<<converged<<" sweeps "<<sweep<<endl;
  return converged?0:2;
 }catch(const exception& e){cerr<<e.what()<<endl;return 1;}
}
