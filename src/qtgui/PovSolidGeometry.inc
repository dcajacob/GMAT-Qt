// SPDX-License-Identifier: Apache-2.0
// Private implementation fragment: included after the POV importer's model types.
namespace PovSolid {
struct Vertex {
   osg::Vec3d point,normal,weights;
   Vertex interpolate(const Vertex &other,double fraction) const {
      return {point+(other.point-point)*fraction,
              normal+(other.normal-normal)*fraction,
              weights+(other.weights-weights)*fraction};
   }
};
struct Polygon {
   std::vector<Vertex> vertices;
   std::shared_ptr<const std::array<Material,3>> materials;
   QString file;
   int line=1;
   bool smooth=false;
   void invert() {
      std::reverse(vertices.begin(),vertices.end());
      for (auto &vertex:vertices) vertex.normal=-vertex.normal;
   }
};
struct Plane { osg::Vec3d normal; double offset=0; };
using Polygons=std::vector<Polygon>;
struct Solid { Polygons polygons; bool outside=false; };

[[noreturn]] void fail(const Model &model,const QString &message)
{
   sourceError(Token{{},model.line,Token::Word,model.file},message);
}
double length(const osg::Vec3d &v) { return std::hypot(v.x(),v.y(),v.z()); }
std::optional<Plane> planeOf(const Polygon &polygon,double epsilon)
{
   if (polygon.vertices.size()<3) return {};
   const auto &first=polygon.vertices.front().point;
   for (size_t i=2;i<polygon.vertices.size();++i) {
      auto n=(polygon.vertices[i-1].point-first)^(polygon.vertices[i].point-first);
      const double magnitude=length(n);
      if (magnitude>epsilon*epsilon && std::isfinite(magnitude)) {
         n/=magnitude; return Plane{n,n*first};
      }
   }
   return {};
}
double tolerance(const Polygons &a,const Polygons &b={})
{
   osg::Vec3d low(0,0,0),high(0,0,0);
   bool first=true; double coordinate=0;
   for (const auto *polygons:{&a,&b}) for (const auto &polygon:*polygons)
      for (const auto &vertex:polygon.vertices) {
         if (first) { low=high=vertex.point; first=false; }
         for (int axis=0;axis<3;++axis) {
            low[axis]=std::min(low[axis],vertex.point[axis]);
            high[axis]=std::max(high[axis],vertex.point[axis]);
            coordinate=std::max(coordinate,std::abs(vertex.point[axis]));
         }
      }
   const double extent=std::max({high.x()-low.x(),high.y()-low.y(),high.z()-low.z()});
   return std::max(std::numeric_limits<double>::min(),
         extent*PovEpsilon+coordinate*64*std::numeric_limits<double>::epsilon());
}
void append(Polygons &target,Polygons values)
{
   target.insert(target.end(),std::make_move_iterator(values.begin()),std::make_move_iterator(values.end()));
}
void appendPiece(Polygons &target,Polygon polygon,std::vector<Vertex> vertices,double epsilon)
{
   polygon.vertices.clear();
   for (auto &vertex:vertices)
      if (polygon.vertices.empty() || length(vertex.point-polygon.vertices.back().point)>epsilon)
         polygon.vertices.push_back(std::move(vertex));
   if (polygon.vertices.size()>1 && length(polygon.vertices.front().point-polygon.vertices.back().point)<=epsilon)
      polygon.vertices.pop_back();
   if (planeOf(polygon,epsilon)) target.push_back(std::move(polygon));
}
void split(Polygon polygon,const Plane &plane,Polygons &coplanarFront,Polygons &coplanarBack,
           Polygons &front,Polygons &back,double epsilon,ImportControl &control)
{
   control.check();
   std::vector<int> sides;
   std::vector<double> distances;
   int type=0;
   for (const auto &vertex:polygon.vertices) {
      const double distance=plane.normal*vertex.point-plane.offset;
      const int side=distance>epsilon ? 1 : distance < -epsilon ? 2 : 0;
      sides.push_back(side); distances.push_back(distance); type|=side;
   }
   if (type==0) {
      const auto own=planeOf(polygon,epsilon);
      if (own) (plane.normal*own->normal>=0 ? coplanarFront : coplanarBack).push_back(std::move(polygon));
   } else if (type==1) front.push_back(std::move(polygon));
   else if (type==2) back.push_back(std::move(polygon));
   else {
      std::vector<Vertex> f,b;
      for (size_t i=0;i<polygon.vertices.size();++i) {
         const size_t next=(i+1)%polygon.vertices.size();
         if (sides[i]!=2) f.push_back(polygon.vertices[i]);
         if (sides[i]!=1) b.push_back(polygon.vertices[i]);
         if ((sides[i]|sides[next])==3) {
            const double fraction=std::clamp(distances[i]/(distances[i]-distances[next]),0.0,1.0);
            auto vertex=polygon.vertices[i].interpolate(polygon.vertices[next],fraction);
            f.push_back(vertex); b.push_back(std::move(vertex));
         }
      }
      appendPiece(front,polygon,std::move(f),epsilon);
      appendPiece(back,std::move(polygon),std::move(b),epsilon);
   }
}

// Explicit work stacks keep convex meshes with many successive clipping planes
// from consuming one C++ call frame per plane. All long sweeps remain cancelable.
class Tree {
   static constexpr size_t absent=std::numeric_limits<size_t>::max();
   struct Branch {
      std::optional<Plane> plane;
      Polygons polygons;
      size_t front=absent,back=absent;
   };
   struct Job { size_t branch; Polygons polygons; };
   std::vector<Branch> branches;
   ImportControl &control;
   double epsilon;
public:
   Tree(Polygons polygons,ImportControl &control,double epsilon) : control(control),epsilon(epsilon) {
      build(std::move(polygons));
   }
   void build(Polygons polygons) {
      if (polygons.empty()) return;
      if (branches.empty()) branches.emplace_back();
      std::vector<Job> jobs; jobs.push_back({0,std::move(polygons)});
      while (!jobs.empty()) {
         control.check();
         auto job=std::move(jobs.back()); jobs.pop_back();
         const size_t index=job.branch;
         if (!branches[index].plane) branches[index].plane=planeOf(job.polygons.front(),epsilon);
         if (!branches[index].plane)
            throw std::runtime_error((sourceLocation(job.polygons.front().file,job.polygons.front().line)+
                  ": Degenerate POV CSG polygon.").toStdString());
         const auto plane=*branches[index].plane;
         Polygons front,back;
         for (auto &polygon:job.polygons)
            split(std::move(polygon),plane,branches[index].polygons,branches[index].polygons,
                  front,back,epsilon,control);
         for (bool isFront:{false,true}) {
            auto &values=isFront ? front : back;
            if (values.empty()) continue;
            size_t child=isFront ? branches[index].front : branches[index].back;
            if (child==absent) {
               child=branches.size();
               if (isFront) branches[index].front=child; else branches[index].back=child;
               branches.emplace_back();
            }
            jobs.push_back({child,std::move(values)});
         }
      }
   }
   Polygons clip(Polygons polygons) const {
      if (branches.empty()) return polygons;
      Polygons result;
      std::vector<Job> jobs; jobs.push_back({0,std::move(polygons)});
      while (!jobs.empty()) {
         control.check();
         auto job=std::move(jobs.back()); jobs.pop_back();
         const auto &branch=branches[job.branch];
         Polygons front,back;
         for (auto &polygon:job.polygons)
            split(std::move(polygon),*branch.plane,front,back,front,back,epsilon,control);
         if (branch.front==absent) append(result,std::move(front));
         else if (!front.empty()) jobs.push_back({branch.front,std::move(front)});
         if (branch.back!=absent && !back.empty()) jobs.push_back({branch.back,std::move(back)});
      }
      return result;
   }
   void clipTo(const Tree &other) {
      for (auto &branch:branches) { control.check(); branch.polygons=other.clip(std::move(branch.polygons)); }
   }
   void invert() {
      for (auto &branch:branches) {
         control.check();
         branch.plane->normal=-branch.plane->normal; branch.plane->offset=-branch.plane->offset;
         for (auto &polygon:branch.polygons) { control.check(); polygon.invert(); }
         std::swap(branch.front,branch.back);
      }
   }
   Polygons polygons() const {
      Polygons result;
      for (const auto &branch:branches) {
         control.check(); result.insert(result.end(),branch.polygons.begin(),branch.polygons.end());
      }
      return result;
   }
};
void invert(Solid &solid,ImportControl &control)
{
   solid.outside=!solid.outside;
   for (auto &polygon:solid.polygons) { control.check(); polygon.invert(); }
}
Solid combine(Solid left,Solid right,Model::Kind operation,ImportControl &control)
{
   const bool difference=operation==Model::Kind::Difference;
   const bool intersection=operation==Model::Kind::Intersection;
   // Empty boundaries can mean empty space or all space after inversion.
   // Keep that membership separate from the finite boundary polygons.
   if (left.polygons.empty()) {
      if (difference) { if (!left.outside) return {}; invert(right,control); return right; }
      return intersection ? (left.outside ? std::move(right) : Solid{}) :
                            (left.outside ? Solid{{},true} : std::move(right));
   }
   if (right.polygons.empty()) {
      if (difference) return right.outside ? Solid{} : std::move(left);
      return intersection ? (right.outside ? std::move(left) : Solid{}) :
                            (right.outside ? Solid{{},true} : std::move(left));
   }
   const bool outside=difference ? left.outside && !right.outside :
         intersection ? left.outside && right.outside : left.outside || right.outside;
   const double epsilon=tolerance(left.polygons,right.polygons);
   Tree a(std::move(left.polygons),control,epsilon),b(std::move(right.polygons),control,epsilon);
   if (difference) {
      a.invert(); a.clipTo(b); b.clipTo(a); b.invert(); b.clipTo(a); b.invert();
      a.build(b.polygons()); a.invert();
   } else if (intersection) {
      a.invert(); b.clipTo(a); b.invert(); a.clipTo(b); b.clipTo(a);
      a.build(b.polygons()); a.invert();
   } else {
      a.clipTo(b); b.clipTo(a); b.invert(); b.clipTo(a); b.invert(); a.build(b.polygons());
   }
   return {a.polygons(),outside};
}

// Orient a closed authored mesh from topology and its specified inside ray.
// Smooth shading normals retain their authored values; only geometric winding
// determines solid membership. Flat shading is recomputed after orientation.
void orientMesh(Solid &solid,const osg::Vec3d &inside,const Model &model,ImportControl &control)
{
   using Point=std::array<double,3>;
   using Edge=std::pair<Point,Point>;
   using Incidence=std::pair<size_t,bool>;
   std::map<Edge,std::vector<Incidence>> edges;
   for (size_t i=0;i<solid.polygons.size();++i) {
      control.check();
      const auto &vertices=solid.polygons[i].vertices;
      for (size_t j=0;j<3;++j) {
         const auto &v=vertices[j].point; const auto &w=vertices[(j+1)%3].point;
         Point a{v.x(),v.y(),v.z()},b{w.x(),w.y(),w.z()};
         const bool forward=a<b;
         if (a==b) fail(model,"A CSG mesh has a collapsed edge.");
         edges[forward ? Edge{a,b} : Edge{b,a}].push_back({i,forward});
      }
   }
   std::vector<std::vector<Incidence>> neighbors(solid.polygons.size());
   for (const auto &entry:edges) {
      control.check(); const auto &incident=entry.second;
      if (incident.size()!=2) fail(model,"CSG mesh operands need a closed, consistently connected surface (two faces per edge).");
      const bool same=incident[0].second==incident[1].second;
      neighbors[incident[0].first].push_back({incident[1].first,same});
      neighbors[incident[1].first].push_back({incident[0].first,same});
   }
   std::vector<int> reversed(solid.polygons.size(),-1);
   std::vector<std::vector<size_t>> components;
   for (size_t start=0;start<reversed.size();++start) {
      if (reversed[start]!=-1) continue;
      components.push_back({start}); reversed[start]=0;
      auto &component=components.back();
      for (size_t cursor=0;cursor<component.size();++cursor) {
         control.check(); const size_t face=component[cursor];
         for (const auto &[neighbor,same]:neighbors[face]) {
            const int wanted=reversed[face]^int(same);
            if (reversed[neighbor]==-1) { reversed[neighbor]=wanted; component.push_back(neighbor); }
            else if (reversed[neighbor]!=wanted) fail(model,"The CSG mesh surface is not orientable.");
         }
      }
   }
   for (size_t i=0;i<reversed.size();++i)
      if (reversed[i]) std::reverse(solid.polygons[i].vertices.begin(),solid.polygons[i].vertices.end());
   const double epsilon=tolerance(solid.polygons);
   const double magnitude=length(inside);
   if (!std::isfinite(magnitude) || magnitude==0) fail(model,"A CSG mesh needs a finite, nonzero inside_vector.");
   const auto direction=inside/magnitude;
   auto isInside=[&](const osg::Vec3d &point) {
      std::vector<std::pair<double,bool>> hits;
      for (const auto &polygon:solid.polygons) {
         control.check();
         const auto &v=polygon.vertices;
         const auto e1=v[1].point-v[0].point,e2=v[2].point-v[0].point;
         const auto p=direction^e2; const double determinant=e1*p;
         if (std::abs(determinant)<=length(e1^e2)*64*std::numeric_limits<double>::epsilon()) continue;
         const auto t=point-v[0].point; const double u=(t*p)/determinant;
         const auto q=t^e1; const double vcoord=(direction*q)/determinant;
         if (u < -PovEpsilon || vcoord < -PovEpsilon || u+vcoord>1+PovEpsilon) continue;
         const double distance=(e2*q)/determinant;
         if (distance>epsilon) hits.push_back({distance,determinant>0});
      }
      std::sort(hits.begin(),hits.end()); size_t crossings=0;
      for (size_t i=0;i<hits.size();) {
         control.check(); size_t next=i+1; bool positive=hits[i].second,negative=!positive;
         while (next<hits.size() && hits[next].first-hits[i].first<=epsilon) {
            positive|=hits[next].second; negative|=!hits[next].second; ++next;
         }
         if (!(positive && negative)) ++crossings;
         i=next;
      }
      return crossings%2!=0;
   };
   for (const auto &component:components) {
      const auto &first=solid.polygons[component.front()]; const auto plane=planeOf(first,epsilon);
      if (!plane) fail(model,"Degenerate triangle in CSG mesh.");
      const auto point=(first.vertices[0].point+first.vertices[1].point+first.vertices[2].point)/3.0;
      if (isInside(point+plane->normal*(8*epsilon)))
         for (auto index:component) {
            control.check(); auto &vertices=solid.polygons[index].vertices;
            std::reverse(vertices.begin(),vertices.end());
         }
   }
   for (auto &polygon:solid.polygons) if (!polygon.smooth) {
      const auto plane=planeOf(polygon,epsilon);
      if (!plane) fail(model,"Degenerate triangle in CSG mesh.");
      for (auto &vertex:polygon.vertices) vertex.normal=plane->normal;
   }
}

class Builder {
   ImportControl &control;
   int depth=0;
   Solid evaluate(const Node &model,const Material &inherited,const Material *rootOverride=nullptr,
                  const osg::Vec3d *insideOverride=nullptr,bool applyMatrix=true,bool boundaryOnly=true) {
      control.check(); QScopedValueRollback<int> nesting(depth,depth+1);
      if (depth>128) fail(*model,"CSG object hierarchy exceeds the model reader's nesting limit.");
      const Material &rootMaterial=rootOverride ? *rootOverride : model->material;
      Material material=inherited; material.overlay(rootMaterial);
      Solid result;
      if (model->instance) {
         const auto *inside=insideOverride ? insideOverride : model->insideVector ? &*model->insideVector : nullptr;
         result=evaluate(model->children.at(0),inherited,&rootMaterial,inside,true,boundaryOnly);
      } else if (model->kind==Model::Kind::Empty) {
         // Inversion below can turn an intentional empty solid into full space.
      } else if (model->kind==Model::Kind::Primitive || model->kind==Model::Kind::Mesh) {
         if (!model->closed) fail(*model,"Open cylinders and cones cannot supply a closed tessellated CSG operand.");
         const auto &faces=boundaryOnly && model->solidFaces ? *model->solidFaces : model->faces;
         for (const auto &face:faces) {
            control.check(); Polygon polygon; polygon.file=face.file; polygon.line=face.line; polygon.smooth=face.smooth;
            Material surface=material; surface.overlay(face.material);
            auto materials=std::make_shared<std::array<Material,3>>();
            const auto n=normal((face.vertices[1]-face.vertices[0])^(face.vertices[2]-face.vertices[0]),face.file,face.line);
            for (int i=0;i<3;++i) {
               (*materials)[i]=surface;
               if (face.cornerMaterials) (*materials)[i].overlay((*face.cornerMaterials)[i]);
               const auto &point=face.vertices[i];
               for (int axis=0;axis<3;++axis)
                  if (!std::isfinite(point[axis]) || std::abs(point[axis])>std::numeric_limits<float>::max())
                     fail(*model,"A CSG vertex exceeds display precision.");
               const auto shading=face.smooth ? normal(face.normals[i],face.file,face.line) : n;
               polygon.vertices.push_back({point,osg::Vec3d(shading.x(),shading.y(),shading.z()),
                     osg::Vec3d(i==0 ? 1.0 : 0.0,i==1 ? 1.0 : 0.0,i==2 ? 1.0 : 0.0)});
            }
            polygon.materials=std::move(materials); result.polygons.push_back(std::move(polygon));
         }
         if (model->kind==Model::Kind::Mesh) {
            const auto *inside=insideOverride ? insideOverride : model->insideVector ? &*model->insideVector : nullptr;
            if (!inside) fail(*model,"A mesh used as a CSG solid needs inside_vector.");
            orientMesh(result,*inside,*model,control);
         }
      } else if (isOperation(model->kind)) {
         bool first=true;
         const bool combineBoundary=boundaryOnly || model->kind!=Model::Kind::Union;
         for (const auto &child:model->children) {
            auto value=evaluate(child,material,nullptr,nullptr,true,combineBoundary);
            if (first) { result=std::move(value); first=false; }
            else if (!combineBoundary) {
               result.outside|=value.outside;
               append(result.polygons,std::move(value.polygons));
            }
            else result=combine(std::move(result),std::move(value),model->kind,control);
         }
      } else fail(*model,"CSG operands need solid primitives or closed meshes; individual triangles are patches.");
      if (model->inverse) invert(result,control);
      if (applyMatrix && !model->matrix.isIdentity()) {
         osg::Matrixd inverse;
         if (!inverse.invert(model->matrix)) fail(*model,"A CSG transform must be nonsingular.");
         auto transformNormal=[&](const osg::Vec3d &n) {
            return osg::Vec3d(inverse(0,0)*n.x()+inverse(0,1)*n.y()+inverse(0,2)*n.z(),
                             inverse(1,0)*n.x()+inverse(1,1)*n.y()+inverse(1,2)*n.z(),
                             inverse(2,0)*n.x()+inverse(2,1)*n.y()+inverse(2,2)*n.z());
         };
         const double epsilon=tolerance(result.polygons);
         for (auto &polygon:result.polygons) {
            control.check(); const auto plane=planeOf(polygon,epsilon);
            if (!plane) fail(*model,"Degenerate transformed CSG polygon.");
            const auto outward=transformNormal(plane->normal);
            for (auto &vertex:polygon.vertices) {
               vertex.point=vertex.point*model->matrix;
               vertex.normal=transformNormal(vertex.normal);
               for (int axis=0;axis<3;++axis)
                  if (!std::isfinite(vertex.point[axis]) || !std::isfinite(vertex.normal[axis]))
                     fail(*model,"A CSG transform exceeds display precision.");
            }
            // A reflected object retains outward geometric winding and its
            // inverse-transpose shading normals before later clipping.
            const auto transformed=planeOf(polygon,tolerance(Polygons{polygon}));
            if (!transformed) fail(*model,"A CSG transform collapses a surface at display precision.");
            if (transformed->normal*outward<0) std::reverse(polygon.vertices.begin(),polygon.vertices.end());
         }
      }
      return result;
   }
   static bool isOperation(Model::Kind kind) {
      return kind==Model::Kind::Union || kind==Model::Kind::Intersection ||
            kind==Model::Kind::Difference || kind==Model::Kind::Merge;
   }
public:
   explicit Builder(ImportControl &control) : control(control) {}
   std::vector<Triangle> faces(const Node &model,const Material &inherited,const Material *rootOverride) {
      const auto solid=evaluate(model,inherited,rootOverride,nullptr,false,false);
      const double epsilon=tolerance(solid.polygons);
      std::vector<Triangle> result;
      for (const auto &polygon:solid.polygons) for (size_t i=2;i<polygon.vertices.size();++i) {
         control.check();
         const size_t indices[3]={0,i-1,i};
         const auto a=polygon.vertices[indices[1]].point-polygon.vertices[0].point;
         const auto b=polygon.vertices[indices[2]].point-polygon.vertices[0].point;
         if (length(a^b)<=epsilon*epsilon) continue;
         Triangle face; face.file=polygon.file; face.line=polygon.line; face.smooth=true;
         face.cornerMaterials=polygon.materials; face.materialWeights.emplace();
         for (int corner=0;corner<3;++corner) {
            const auto &vertex=polygon.vertices[indices[corner]];
            face.vertices[corner]=vertex.point; face.normals[corner]=vertex.normal;
            (*face.materialWeights)[corner]=vertex.weights;
         }
         result.push_back(std::move(face));
      }
      return result;
   }
};
}
