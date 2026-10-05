#include "OrbitRenderer.hpp"
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include "OrbitObjectGuides.hpp"
#include "OrbitVelocity.hpp"
#include <osg/PolygonOffset>
#include "OrbitVectors.hpp"
#include "PlotInterpolation.hpp"
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QSurfaceFormat>
#include <QPainter>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include "OrbitLabels.hpp"
#include "OrbitSegmentLabels.hpp"
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>

namespace {
osg::Vec4 color(const QColor &c) { return {float(c.redF()),float(c.greenF()),float(c.blueF()),1}; }
osg::ref_ptr<osg::Program> markerProgram(bool endpoint)
{
   auto program=new osg::Program;
   program->addShader(new osg::Shader(osg::Shader::VERTEX,"#version 120\nvoid main() { gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex; gl_FrontColor=gl_Color; }"));
   const std::string shape=endpoint ? "float r=length(v); if (r>0.001 && r>abs(v.x*v.y/(r*r))) discard;" : "float r=dot(v,v); if (r>0.25 || r<0.16) discard;";
   program->addShader(new osg::Shader(osg::Shader::FRAGMENT,"#version 120\nvoid main() { vec2 v=gl_PointCoord-vec2(0.5); "+shape+" gl_FragColor=gl_Color; }"));
   return program;
}
osg::ref_ptr<osg::Program> wideLineProgram()
{
   auto program=new osg::Program;
   program->addShader(new osg::Shader(osg::Shader::VERTEX,R"(#version 120
uniform vec2 viewportSize;
uniform float lineWidth;
void main() {
   vec4 p=gl_ModelViewProjectionMatrix*gl_Vertex;
   vec4 q=gl_ModelViewProjectionMatrix*vec4(gl_MultiTexCoord0.xyz,1.0);
   vec2 delta=(q.xy/q.w-p.xy/p.w)*viewportSize;
   vec2 direction=length(delta)>0.000001 ? normalize(delta) : vec2(1.0,0.0);
   vec2 offset=(vec2(-direction.y,direction.x)*gl_MultiTexCoord0.w-direction)*lineWidth/viewportSize;
   p.xy+=offset*p.w;
   gl_Position=p; gl_FrontColor=gl_Color;
})"));
   program->addShader(new osg::Shader(osg::Shader::FRAGMENT,"#version 120\nvoid main() { gl_FragColor=gl_Color; }"));
   return program;
}
// Clip before dividing by homogeneous w in the width shader. This also keeps
// segments through/behind the camera from flipping their screen-space direction.
bool clipLine(osg::Vec3d &a,osg::Vec3d &b,osg::Vec4 &ca,osg::Vec4 &cb,const osg::Matrixd &projection)
{
   for (const int side:{1,-1}) {
      const auto p=osg::Vec4d(a.x(),a.y(),a.z(),1)*projection,q=osg::Vec4d(b.x(),b.y(),b.z(),1)*projection;
      const double da=p.w()+side*p.z(),db=q.w()+side*q.z();
      if (da<0 && db<0) return false;
      if (da<0 || db<0) {
         const double fraction=da/(da-db); const auto position=a+(b-a)*fraction;
         const auto tint=ca+(cb-ca)*float(fraction);
         if (da<0) { a=position; ca=tint; } else { b=position; cb=tint; }
      }
   }
   return true;
}
osg::ref_ptr<osg::Image> readTexture(const QString &path)
{
   if (path.isEmpty()) return {};
   const QImage pixels=QImage(path).convertToFormat(QImage::Format_RGBA8888).mirrored();
   if (pixels.isNull()) return {};
   osg::ref_ptr<osg::Image> image=new osg::Image;
   image->allocateImage(pixels.width(),pixels.height(),1,GL_RGBA,GL_UNSIGNED_BYTE);
   std::memcpy(image->data(),pixels.constBits(),pixels.sizeInBytes());
   return image;
}
class TextureReader final : public osgDB::ReadFileCallback {
public:
   osgDB::ReaderWriter::ReadResult readImage(const std::string &filename,const osgDB::Options *options) override {
      const auto path=osgDB::findDataFile(filename,options);
      auto image=readTexture(QString::fromStdString(path));
      return image ? osgDB::ReaderWriter::ReadResult(image) : osgDB::ReaderWriter::ReadResult::FILE_NOT_FOUND;
   }
};
void isolateArrays(osg::Drawable *drawable)
{
   drawable->setUseDisplayList(false);
   drawable->setUseVertexBufferObjects(true);
   drawable->setUseVertexArrayObject(true);
}
class ModelArrays final : public osg::NodeVisitor {
public:
   ModelArrays() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}
   void apply(osg::Geode &node) override {
      for (unsigned i=0;i<node.getNumDrawables();++i) isolateArrays(node.getDrawable(i));
      traverse(node);
   }
};
osg::ref_ptr<osg::Node> readModel(const QString &path,bool normalize)
{
   // Include build dependencies and an adjacent deployed plugin directory.
   auto &paths=osgDB::Registry::instance()->getLibraryFilePathList();
   for (const auto &candidate:{QCoreApplication::applicationDirPath()+"/osgPlugins-"+QString(osgGetVersion()),QString(GMAT_QT_OSG_PLUGIN_DIR)})
      if (QDir(candidate).exists() && std::find(paths.begin(),paths.end(),candidate.toStdString())==paths.end()) paths.push_back(candidate.toStdString());
   osg::ref_ptr<osgDB::Options> options=new osgDB::Options;
   options->setOptionString("noRotation");
   options->setReadFileCallback(new TextureReader);
   options->getDatabasePathList().push_back(QFileInfo(path).absolutePath().toStdString());
   auto model=osgDB::readRefNodeFile(path.toStdString(),options);
   if (!model) { qWarning().noquote()<<"Qt orbit: could not load model"<<path<<"; using the default body or marker."; return {}; }
   ModelArrays arrays; model->accept(arrays);
   osg::ComputeBoundsVisitor bounds; model->accept(bounds);
   const auto box=bounds.getBoundingBox();
   if (!box.valid() || box.radius()<=0) return {};
   // Celestial-body meshes retain their physical dimensions. Only spacecraft
   // use wx's normalized, exaggerated display-size convention.
   if (!normalize) return model;
   auto normalized=new osg::MatrixTransform;
   normalized->setMatrix(osg::Matrixd::translate(-osg::Vec3d(box.center()))*osg::Matrixd::scale(1.0/box.radius(),1.0/box.radius(),1.0/box.radius()));
   normalized->addChild(model);
   return normalized;
}
osg::ref_ptr<osg::Geode> sphere(double radius,const QString &path,const QColor &fallback)
{
   constexpr int columns=128,rows=64;
   constexpr double pi=3.14159265358979323846;
   auto vertices=new osg::Vec3Array;
   auto normals=new osg::Vec3Array;
   auto uv=new osg::Vec2Array;
   for (int row=0;row<=rows;++row) for (int col=0;col<=columns;++col) {
      const double latitude=pi*(double(row)/rows-.5),longitude=2*pi*(double(col)/columns-.5);
      const osg::Vec3 normal(std::cos(latitude)*std::cos(longitude),std::cos(latitude)*std::sin(longitude),std::sin(latitude));
      normals->push_back(normal); vertices->push_back(normal*radius);
      uv->push_back(osg::Vec2(double(col)/columns,double(row)/rows));
   }
   auto mesh=new osg::Geometry; isolateArrays(mesh);
   mesh->setVertexArray(vertices); mesh->setNormalArray(normals,osg::Array::BIND_PER_VERTEX);
   mesh->setTexCoordArray(0,uv);
   auto triangles=new osg::DrawElementsUInt(GL_TRIANGLES);
   for (int row=0;row<rows;++row) for (int col=0;col<columns;++col) {
      const unsigned a=row*(columns+1)+col,b=a+columns+1;
      for (const auto index:{a,a+1,b,a+1,b+1,b}) triangles->push_back(index);
   }
   mesh->addPrimitiveSet(triangles);
   auto colors=new osg::Vec4Array;
   // Use Qt's deployed image readers, independent of OSG's optional image plugins.
   auto image=readTexture(path);
   colors->push_back(image ? osg::Vec4(1,1,1,1) : color(fallback));
   mesh->setColorArray(colors,osg::Array::BIND_OVERALL);
   if (image) {
      auto texture=new osg::Texture2D(image);
      texture->setWrap(osg::Texture::WRAP_S,osg::Texture::REPEAT);
      texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
      texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR_MIPMAP_LINEAR);
      mesh->getOrCreateStateSet()->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
   }
   auto node=new osg::Geode; node->addDrawable(mesh); return node;
}
}

bool orbitModelFileReadable(const QString &path) { return !path.isEmpty() && readModel(path,true).valid(); }

struct OrbitRenderer::Scene
{
   struct Curve {
      osg::ref_ptr<osg::Group> root=new osg::Group;
      osg::ref_ptr<osg::Geode> track=new osg::Geode;
      osg::ref_ptr<osg::Geode> markers=new osg::Geode;
      osg::ref_ptr<osg::MatrixTransform> body=new osg::MatrixTransform;
      QString texture,modelPath;
      osg::ref_ptr<osg::MatrixTransform> modelPose=new osg::MatrixTransform;
      bool modelLoaded=false;
      double radius=-1,assetExtent=1;
      Curve() { root->addChild(track); root->addChild(body); root->addChild(markers); }
   };
   static osg::Matrixd modelTransform(const PlotCurve &source) {
      const auto &r=source.modelRotation,&offset=source.modelOffset;
      constexpr double radians=3.14159265358979323846/180;
      const double scale=source.modelScale*(source.radius==0 ? 1000 : 1);
      const auto rotation=osg::Matrixd::rotate(r[0]*radians,osg::Vec3d(1,0,0),r[1]*radians,osg::Vec3d(0,1,0),r[2]*radians,osg::Vec3d(0,0,1));
      return source.radius==0 ? osg::Matrixd::translate(offset[0],offset[1],offset[2])*osg::Matrixd::scale(scale,scale,scale)*rotation :
         osg::Matrixd::scale(scale,scale,scale)*rotation*osg::Matrixd::translate(offset[0],offset[1],offset[2]);
   }
   std::shared_ptr<PlotModel> model;
   osgViewer::Viewer viewer;
   osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> context;
   osg::ref_ptr<osg::Group> root=new osg::Group;
   osg::ref_ptr<osg::Geode> guides=new osg::Geode;
   osg::ref_ptr<osg::Geode> sky=new osg::Geode;
   osg::ref_ptr<osg::LightSource> illumination=new osg::LightSource;
   std::map<int,Curve> curves;
   QMap<QString,OrbitObjectBounds> vectorBounds;
   osg::ref_ptr<osg::Program> markerShaders[2]={markerProgram(false),markerProgram(true)};
   osg::ref_ptr<osg::Program> widthShader=wideLineProgram();
   double nativeLineWidthLimit=1;
   double zoom=1,yaw=.55,pitch=.45;
   QPointF pan;
   quint64 frame=std::numeric_limits<quint64>::max();
   explicit Scene(std::shared_ptr<PlotModel> value) : model(std::move(value)) {
      viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
      viewer.setLightingMode(osg::View::NO_LIGHT);
      illumination->getLight()->setLightNum(0);
      illumination->getLight()->setAmbient(osg::Vec4(.12,.12,.12,1));
      illumination->getLight()->setDiffuse(osg::Vec4(.9,.9,.9,1));
      illumination->setStateSetModes(*root->getOrCreateStateSet(),osg::StateAttribute::ON);
      root->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::ON);
      root->getOrCreateStateSet()->setMode(GL_DEPTH_TEST,osg::StateAttribute::ON);
      root->addChild(illumination);
      viewer.getCamera()->setClearColor(osg::Vec4(.015,.025,.045,1));
      viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
      // Point markers have a zero-size world-space bound but a visible pixel size.
      viewer.getCamera()->setSmallFeatureCullingPixelSize(-1);
      root->addChild(guides);
      auto *skyState=sky->getOrCreateStateSet();
      skyState->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
      skyState->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS,0,1,false));
      skyState->setRenderBinDetails(-100,"RenderBin");
      root->addChild(sky);
      viewer.setSceneData(root);
   }
   void prepareCurve(int key,const PlotCurve &source) {
      auto added=curves.try_emplace(key); auto &curve=added.first->second;
      if (added.second) root->addChild(curve.root);
         if (curve.radius!=source.radius || curve.texture!=source.texturePath || curve.modelPath!=source.modelPath) {
            curve.body->removeChildren(0,curve.body->getNumChildren());
            if (source.name=="Sun") curve.body->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
            curve.modelPose->removeChildren(0,curve.modelPose->getNumChildren());
            curve.modelLoaded=false;
            if (!source.modelPath.isEmpty()) {
               auto model=readModel(source.modelPath,source.radius==0);
               if (model) { curve.modelPose->addChild(model); curve.body->addChild(curve.modelPose); curve.modelLoaded=true; curve.assetExtent=model->getBound().center().length()+model->getBound().radius(); }
            }
            if (source.radius>0 && !curve.modelLoaded) curve.body->addChild(sphere(source.radius,source.texturePath,source.color));
            curve.radius=source.radius; curve.texture=source.texturePath; curve.modelPath=source.modelPath;
         }
   }
   void synchronize(int width,int height,double pixelRatio) {
      double extent=1;
      OrbitSceneBounds bounds;
      QMap<QString,OrbitObjectBounds> objects;
      for (auto it=model->curves.cbegin();it!=model->curves.cend();++it) {
         const auto &source=it.value(); prepareCurve(it.key(),source);
         auto &prepared=curves.at(it.key());
         if (prepared.modelLoaded) {
            prepared.modelPose->setMatrix(modelTransform(source));
            const auto sphere=prepared.modelPose->getBound();
            objects.insert(source.name,{sphere.center(),sphere.radius()>0 ? sphere.radius() : 1});
         } else objects.insert(source.name,{{},source.radius>0 ? source.radius : 1});
         if (!source.drawsContent()) continue;
         const double scale=std::abs(source.modelScale)*(source.radius==0 ? 1000 : 1);
         const double offset=std::hypot(source.modelOffset[0],source.modelOffset[1],source.modelOffset[2]);
         const double radius=prepared.modelLoaded ? prepared.assetExtent*scale+offset*(source.radius==0 ? scale : 1) : source.radius;
         const double guideRadius=orbitObjectGuideRadius(source,prepared.modelLoaded ? prepared.modelPose->getBound().radius() : 0)*(source.objectXYPlane ? 15 : source.objectGrid ? 1 : 0);
         const double visibleRadius=std::max(source.showObject ? radius : 0,guideRadius);
         for (const auto &p:source.points) {
            extent=std::max(extent,std::hypot(p.x,p.y,p.z)+visibleRadius);
            bounds.include(p.x,p.y,p.z,visibleRadius);
            if (source.objectVelocity) if (const auto velocity=orbitVelocitySegment(p)) {
               extent=std::max(extent,velocity->end.length());
               bounds.include(velocity->end.x(),velocity->end.y(),velocity->end.z(),0);
            }
         }
      }
      vectorBounds=objects;
      for (const auto &vector:model->vectors) for (const auto &sample:vector.samples)
         if (const auto arrow=orbitVectorArrow(*model,vector,sample,&objects)) for (const auto &point:{arrow->start,arrow->end}) {
            extent=std::max(extent,point.length()); bounds.include(point.x(),point.y(),point.z(),0);
         }
      // Preserve the orbit controls and stable replay framing in either projection.
      const double aspect=double(width)/height;
      const auto camera=orbitCamera(*model,frame,yaw,pitch,extent,aspect,&bounds,&objects);
      const auto &target=camera.target,&right=camera.right,&up=camera.up,&outward=camera.outward;
      const double distance=camera.distance,viewExtent=camera.extent;
      const double tangent=std::tan(std::clamp(model->fieldOfView,1.0,150.0)*osg::PI/360.0)/zoom;
      const double halfHeight=model->perspective ? distance*tangent : viewExtent/zoom/std::min(1.0,aspect),halfWidth=halfHeight*aspect;
      osg::Vec4 lightPosition(outward.x(),outward.y(),outward.z(),0);
      for (const auto &source:model->curves) {
         const auto *last=orbitObjectPose(source,frame);
         if (model->sunlight && last && last->hasSun) {
            lightPosition=osg::Vec4(last->sunPosition[0],last->sunPosition[1],last->sunPosition[2],1);
            break;
         }
      }
      illumination->getLight()->setPosition(lightPosition);
      const osg::Vec3d center=target+right*(-pan.x()*2*halfWidth/width)+up*(pan.y()*2*halfHeight/height);
      const double farPlane=std::max(extent*10,distance+center.length()+extent*4);
      viewer.getCamera()->setViewMatrixAsLookAt(center+outward*distance,center,up);
      const double nearPlane=std::max(1e-6,std::min(extent*.01,distance*.001));
      if (model->perspective)
         viewer.getCamera()->setProjectionMatrixAsFrustum(-halfWidth*nearPlane/distance,halfWidth*nearPlane/distance,
            -halfHeight*nearPlane/distance,halfHeight*nearPlane/distance,nearPlane,farPlane);
      else viewer.getCamera()->setProjectionMatrixAsOrtho(-halfWidth,halfWidth,-halfHeight,halfHeight,nearPlane,farPlane);
      sky->removeDrawables(0,sky->getNumDrawables());
      std::array<double,9> inertialToView={1,0,0,0,1,0,0,0,1};
      const PlotPoint *sample=nullptr;
      for (const auto &source:model->curves)
         if (const auto *pose=orbitObjectPose(source,frame); pose && (!sample || pose->frame>=sample->frame)) sample=pose;
      if (sample) inertialToView=sample->inertialToView;
      auto rotateSky=[&](const std::array<double,3> &value) {
         osg::Vec3d result;
         for (int row=0;row<3;++row) for (int col=0;col<3;++col) result[row]+=inertialToView[row*3+col]*value[col];
         return result;
      };
      const double skyTangent=model->perspective ? tangent : .4663076581549986;
      const double skyOffset=(farPlane-distance)*.9;
      const double skyScale=model->perspective ? (distance+skyOffset)/distance : 1;
      auto skyPosition=[&](const osg::Vec3d &direction) {
         const double forward=-(direction*outward),tangent=skyTangent;
         const double x=(direction*right)/(forward*tangent*aspect),y=(direction*up)/(forward*tangent);
         return center+right*(x*halfWidth*skyScale)+up*(y*halfHeight*skyScale)-outward*skyOffset;
      };
      if (model->starsEnabled && model->starCount>0) {
         osg::ref_ptr<osg::Vec3Array> positions[5];
         for (auto &group:positions) group=new osg::Vec3Array;
         // Orthographic uses a fixed 50-degree sky; perspective uses the camera
         // field of view. Translation never shifts stars at infinity.
         const double tangent=skyTangent;
         const int count=std::min(model->starCount,static_cast<int>(model->starCatalog.stars.size()));
         for (int i=0;i<count;++i) {
            const auto &star=model->starCatalog.stars[i]; osg::Vec3d direction;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col)
               direction[row]+=inertialToView[row*3+col]*star.direction[col];
            const double forward=-(direction*outward);
            if (forward<=0) continue;
            const double x=(direction*right)/(forward*tangent*aspect),y=(direction*up)/(forward*tangent);
            if (std::abs(x)>1 || std::abs(y)>1) continue;
            const int group=static_cast<int>(std::clamp((star.magnitude+1)/2,0.0,4.0));
            positions[group]->push_back(skyPosition(direction));
         }
         for (int group=0;group<5;++group) {
            auto geometry=new osg::Geometry; isolateArrays(geometry);
            geometry->setVertexArray(positions[group]);
            auto colors=new osg::Vec4Array; const float brightness=1.0f-.14f*group;
            colors->push_back({brightness,brightness,brightness,1});
            geometry->setColorArray(colors,osg::Array::BIND_OVERALL);
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS,0,positions[group]->size()));
            geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::Point((2.8-.45*group)*pixelRatio));
            sky->addDrawable(geometry);
         }
      }
      if (model->constellationsEnabled) {
         auto geometry=new osg::Geometry; isolateArrays(geometry);
         auto positions=new osg::Vec3Array;
         for (const auto &segment:model->constellationCatalog.segments) {
            auto a=rotateSky(segment.first),b=rotateSky(segment.second);
            const double fa=-(a*outward),fb=-(b*outward),near=.001;
            if (fa<near && fb<near) continue;
            // Clip against the forward celestial hemisphere before projection.
            // Frustum clipping handles segments crossing a screen edge.
            if (fa<near) a=a+(b-a)*((near-fa)/(fb-fa));
            else if (fb<near) b=b+(a-b)*((near-fb)/(fa-fb));
            positions->push_back(skyPosition(a)); positions->push_back(skyPosition(b));
         }
         geometry->setVertexArray(positions);
         auto colors=new osg::Vec4Array; colors->push_back({.25f,.42f,.62f,1});
         geometry->setColorArray(colors,osg::Array::BIND_OVERALL);
         geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES,0,positions->size()));
         geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(pixelRatio));
         sky->addDrawable(geometry);
      }
      auto guideGeometry=new osg::Geometry; isolateArrays(guideGeometry);
      auto guidePositions=new osg::Vec3Array; auto guideColors=new osg::Vec4Array;
      osg::ref_ptr<osg::Vec3Array> thickPositions=new osg::Vec3Array;
      osg::ref_ptr<osg::Vec4Array> thickNeighbors=new osg::Vec4Array,thickColors=new osg::Vec4Array;
      osg::ref_ptr<osg::Vec3Array> planePositions=new osg::Vec3Array;
      osg::ref_ptr<osg::Vec4Array> planeColors=new osg::Vec4Array;
      auto line=[&](const osg::Vec3d &a,const osg::Vec3d &b,const osg::Vec4 &c) {
         guidePositions->push_back(a); guidePositions->push_back(b);
         guideColors->push_back(c); guideColors->push_back(c);
      };
      if (model->axes) {
         line({0,0,0},{extent,0,0},{.92f,.35f,.31f,1});
         line({0,0,0},{0,extent,0},{.39f,.84f,.53f,1});
         line({0,0,0},{0,0,extent},{.41f,.59f,1,1});
      }
      auto plane=[&](bool ecliptic,const osg::Vec4 &color) {
         constexpr double obliquity=23.439291111*3.14159265358979323846/180;
         auto position=[&](double radius,double angle) {
            const double x=radius*std::cos(angle),y=radius*std::sin(angle);
            return ecliptic ? rotateSky({x,y*std::cos(obliquity),y*std::sin(obliquity)}) : osg::Vec3d(x,y,0);
         };
         for (int ring=1;ring<=4;++ring) for (int i=0;i<96;++i)
            line(position(extent*ring/4,2*3.14159265358979323846*i/96),position(extent*ring/4,2*3.14159265358979323846*(i+1)/96),color);
         for (int i=0;i<12;++i) line({0,0,0},position(extent,2*3.14159265358979323846*i/12),color);
      };
      if (model->xyPlane) plane(false,{.25f,.3f,.42f,1});
      if (model->eclipticPlane) plane(true,{.42f,.3f,.18f,1});
      if (model->sunLine && sample && sample->hasSun) {
         osg::Vec3d sun(sample->sunPosition[0],sample->sunPosition[1],sample->sunPosition[2]);
         if (sun.normalize()>0) line({0,0,0},sun*(extent*1.25),{1,.82f,.2f,1});
      }
      if (model->grid) for (int i=0;i<=5;++i) {
         const double x=halfWidth*(-1+2.0*i/5),y=halfHeight*(-1+2.0*i/5);
         const auto back=center-outward*extent*2;
         line(back+right*x-up*halfHeight,back+right*x+up*halfHeight,{.11f,.14f,.19f,1});
         line(back+up*y-right*halfWidth,back+up*y+right*halfWidth,{.11f,.14f,.19f,1});
      }
      for (const auto &source:model->curves) if (source.visible && source.objectVelocity) {
         const auto first=model->firstVisibleFrame(source,frame);
         for (const auto &point:plotDisplayedPoints(source,frame)) if (point.frame>=first)
            if (const auto velocity=orbitVelocitySegment(point)) line(velocity->start,velocity->end,color(velocity->color));
      }
      for (const auto &source:model->curves) if (source.visible && source.objectAxes) {
         const auto *pose=orbitObjectPose(source,frame); if (!pose) continue;
         const auto axes=orbitObjectAxes(*pose,orbitObjectAxisRadius(source,objects.value(source.name).radius));
         for (const auto &axis:axes) {
            line(axis.start,axis.end,color(pose->color));
            for (const auto &wing:axis.arrow) line(axis.end,wing,color(pose->color));
         }
      }
      const auto guideProjection=viewer.getCamera()->getViewMatrix()*viewer.getCamera()->getProjectionMatrix();
      for (auto it=model->curves.cbegin();it!=model->curves.cend();++it) {
         const auto &source=it.value(); if (!source.visible || (!source.objectGrid && !source.objectXYPlane)) continue;
         const auto *pose=orbitObjectPose(source,frame); if (!pose) continue;
         const auto &prepared=curves.at(it.key());
         const auto geometry=orbitObjectGuides(source,*pose,orbitObjectGuideRadius(source,prepared.modelLoaded ? prepared.modelPose->getBound().radius() : 0));
         for (const auto &edge:geometry.lines) {
            const auto &c=edge.color; osg::Vec4 tint(float(c.redF()),float(c.greenF()),float(c.blueF()),float(c.alphaF()));
            if (edge.width==1) { line(edge.start,edge.end,tint); continue; }
            auto a=edge.start,b=edge.end; auto ca=tint,cb=tint;
            if (!clipLine(a,b,ca,cb,guideProjection)) continue;
            auto vertex=[&](const osg::Vec3d &point,const osg::Vec3d &other,float side,const osg::Vec4 &tint) {
               thickPositions->push_back(point); thickNeighbors->push_back(osg::Vec4(other.x(),other.y(),other.z(),side)); thickColors->push_back(tint);
            };
            vertex(a,b,1,ca); vertex(a,b,-1,ca); vertex(b,a,1,cb);
            vertex(a,b,1,ca); vertex(b,a,1,cb); vertex(b,a,-1,cb);
         }
         for (const auto &triangle:geometry.triangles) for (const auto &point:triangle.points) {
            const auto &c=triangle.color; planePositions->push_back(point); planeColors->push_back(osg::Vec4(float(c.redF()),float(c.greenF()),float(c.blueF()),float(c.alphaF())));
         }
      }
      for (const auto &vector:model->vectors) if (const auto arrow=orbitVectorArrow(*model,vector,frame,&vectorBounds)) {
         const osg::Vec4 tint(float(vector.color.redF()),float(vector.color.greenF()),float(vector.color.blueF()),float(vector.color.alphaF()));
         line(arrow->start,arrow->end,tint); for (const auto &wing:arrow->arrow) line(arrow->end,wing,tint);
      }
      guideGeometry->setVertexArray(guidePositions);
      guideGeometry->setColorArray(guideColors,osg::Array::BIND_PER_VERTEX);
      guideGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES,0,guidePositions->size()));
      guideGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
      guideGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA),osg::StateAttribute::ON);
      guideGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(pixelRatio));
      guides->removeDrawables(0,guides->getNumDrawables()); guides->addDrawable(guideGeometry);
      if (!thickPositions->empty()) {
         auto geometry=new osg::Geometry; isolateArrays(geometry);
         geometry->setVertexArray(thickPositions.get()); geometry->setTexCoordArray(0,thickNeighbors.get()); geometry->setColorArray(thickColors.get(),osg::Array::BIND_PER_VERTEX);
         geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES,0,thickPositions->size()));
         auto *state=geometry->getOrCreateStateSet(); state->setMode(GL_LIGHTING,osg::StateAttribute::OFF); state->setAttributeAndModes(widthShader,osg::StateAttribute::ON);
         state->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA),osg::StateAttribute::ON);
         state->addUniform(new osg::Uniform("viewportSize",osg::Vec2(width*pixelRatio,height*pixelRatio)));
         state->addUniform(new osg::Uniform("lineWidth",float(3*pixelRatio)));
         geometry->setCullingActive(false); guides->addDrawable(geometry);
      }
      if (!planePositions->empty()) {
         auto geometry=new osg::Geometry; isolateArrays(geometry);
         geometry->setVertexArray(planePositions.get()); geometry->setColorArray(planeColors.get(),osg::Array::BIND_PER_VERTEX);
         geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES,0,planePositions->size()));
         auto *state=geometry->getOrCreateStateSet(); state->setMode(GL_LIGHTING,osg::StateAttribute::OFF); state->setMode(GL_CULL_FACE,osg::StateAttribute::OFF);
         state->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA),osg::StateAttribute::ON);
         state->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL,0,1,false),osg::StateAttribute::ON);
         state->setAttributeAndModes(new osg::PolygonOffset(1,1),osg::StateAttribute::ON); state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
         guides->addDrawable(geometry);
      }
      for (auto it=curves.begin();it!=curves.end();) {
         if (!model->curves.contains(it->first)) { root->removeChild(it->second.root); it=curves.erase(it); }
         else ++it;
      }
      for (auto it=model->curves.cbegin();it!=model->curves.cend();++it) {
         const auto &source=it.value();
         auto added=curves.try_emplace(it.key()); auto &curve=added.first->second;
         if (added.second) root->addChild(curve.root);
         curve.root->setNodeMask(source.visible ? ~0u : 0);
         if (!source.visible) continue;
         curve.body->getOrCreateStateSet()->setAttributeAndModes(
            new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,(model->wireframe || source.wireframeObject) ? osg::PolygonMode::LINE : osg::PolygonMode::FILL),
            osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
         if (curve.modelLoaded) {
            curve.modelPose->getOrCreateStateSet()->setMode(GL_NORMALIZE,osg::StateAttribute::ON);
         }
         auto geometry=new osg::Geometry; isolateArrays(geometry);
         auto positions=new osg::Vec3Array; auto colors=new osg::Vec4Array;
         const PlotPoint *last=nullptr;
         unsigned start=0;
         const bool wide=source.lines && source.importedLineWidth.has_value() && source.orbitLineWidth()*pixelRatio>nativeLineWidthLimit;
         // Shader expansion can cross a viewport edge beyond the world-space
         // centerline's bound; let the GPU clip that geometry instead.
         curve.root->setCullingActive(!wide); curve.track->setCullingActive(!wide);
         const auto first=model->firstVisibleFrame(source,frame);
         for (const auto &p:plotDisplayedPoints(source,frame)) {
            if (p.frame<first) continue;
            if (!p.connect && positions->size()>start) {
               if (source.lines && !wide) geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP,start,positions->size()-start));
               start=positions->size();
            }
            positions->push_back(osg::Vec3(p.x,p.y,p.z)); colors->push_back(color(p.color)); last=&p;
         }
         if (source.lines && !wide && positions->size()>start) geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP,start,positions->size()-start));
         geometry->setVertexArray(positions); geometry->setColorArray(colors,osg::Array::BIND_PER_VERTEX);
         geometry->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
         geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(std::min(double(std::numeric_limits<float>::max()),source.orbitLineWidth()*pixelRatio)));
         if (last && source.showObject && source.radius==0 && !curve.modelLoaded) {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS,positions->size()-1,1));
            geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::Point(7*pixelRatio));
         }
         curve.track->removeDrawables(0,curve.track->getNumDrawables()); curve.track->addDrawable(geometry);
         if (wide) {
            osg::ref_ptr<osg::Vec3Array> vertices=new osg::Vec3Array;
            osg::ref_ptr<osg::Vec4Array> neighbors=new osg::Vec4Array,tints=new osg::Vec4Array;
            const auto projection=viewer.getCamera()->getViewMatrix()*viewer.getCamera()->getProjectionMatrix();
            const PlotPoint *previous=nullptr;
            for (const auto &p:plotDisplayedPoints(source,frame)) {
               if (p.frame<first) continue;
               if (previous && p.connect) {
                  osg::Vec3d a(previous->x,previous->y,previous->z),b(p.x,p.y,p.z);
                  auto ca=color(previous->color),cb=color(p.color);
                  if (clipLine(a,b,ca,cb,projection)) {
                     auto vertex=[&](const osg::Vec3d &point,const osg::Vec3d &other,float side,const osg::Vec4 &tint) {
                        vertices->push_back(osg::Vec3(point.x(),point.y(),point.z())); neighbors->push_back(osg::Vec4(other.x(),other.y(),other.z(),side)); tints->push_back(tint);
                     };
                     vertex(a,b,1,ca); vertex(a,b,-1,ca); vertex(b,a,1,cb);
                     vertex(a,b,1,ca); vertex(b,a,1,cb); vertex(b,a,-1,cb);
                  }
               }
               previous=&p;
            }
            if (!vertices->empty()) {
               auto triangles=new osg::Geometry; isolateArrays(triangles);
               triangles->setVertexArray(vertices.get()); triangles->setTexCoordArray(0,neighbors.get()); triangles->setColorArray(tints.get(),osg::Array::BIND_PER_VERTEX);
               triangles->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES,0,vertices->size()));
               auto *state=triangles->getOrCreateStateSet(); state->setMode(GL_LIGHTING,osg::StateAttribute::OFF); state->setAttributeAndModes(widthShader,osg::StateAttribute::ON);
               state->addUniform(new osg::Uniform("viewportSize",osg::Vec2(width*pixelRatio,height*pixelRatio)));
               state->addUniform(new osg::Uniform("lineWidth",float(std::min(double(std::numeric_limits<float>::max()),source.orbitLineWidth()*pixelRatio))));
               curve.track->addDrawable(triangles);
            }
         }
         curve.markers->removeDrawables(0,curve.markers->getNumDrawables());
         const auto markers=model->orbitMarkers(source,frame);
         for (const bool endpoint:{false,true}) {
            osg::ref_ptr<osg::Vec3Array> vertices=new osg::Vec3Array;
            osg::ref_ptr<osg::Vec4Array> tints=new osg::Vec4Array;
            for (const auto &marker:markers) if (marker.endpoint==endpoint) {
               const auto &p=*marker.point; vertices->push_back(osg::Vec3(p.x,p.y,p.z)); tints->push_back(color(p.color));
            }
            if (vertices->empty()) continue;
            auto glyphs=new osg::Geometry; isolateArrays(glyphs);
            glyphs->setVertexArray(vertices.get()); glyphs->setColorArray(tints.get(),osg::Array::BIND_PER_VERTEX);
            glyphs->addPrimitiveSet(new osg::DrawArrays(GL_POINTS,0,vertices->size()));
            auto *state=glyphs->getOrCreateStateSet(); state->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
            state->setTextureAttributeAndModes(0,new osg::PointSprite,osg::StateAttribute::ON);
            state->setMode(GL_POINT_SPRITE,osg::StateAttribute::ON);
            state->setAttributeAndModes(new osg::Point(std::max(1.0,double(source.orbitMarkerSize)*pixelRatio)));
            state->setAttributeAndModes(markerShaders[endpoint ? 1 : 0],osg::StateAttribute::ON);
            curve.markers->addDrawable(glyphs);
         }
         curve.body->setNodeMask(last && source.showObject ? ~0u : 0);
         if (last) {
            osg::Matrixd transform;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) transform(col,row)=last->bodyToView[row*3+col];
            transform.setTrans(last->x,last->y,last->z); curve.body->setMatrix(transform);

         }
      }
   }
};

OrbitRenderer::OrbitRenderer(std::shared_ptr<PlotModel> data,QWidget *parent) : QOpenGLWidget(parent),scene(new Scene(std::move(data)))
{
   // Multisampled widget framebuffers trigger Intel Iris Xe GPU resets when
   // OSG and Qt share this context. Keep hardware rendering without MSAA.
   QSurfaceFormat format; format.setVersion(2,1); format.setDepthBufferSize(24); format.setSamples(0); setFormat(format);
   setAttribute(Qt::WA_TransparentForMouseEvents);
   setObjectName("orbitRenderer");
}
OrbitRenderer::~OrbitRenderer() { if (context()) disconnect(context(),nullptr,this,nullptr); releaseGraphics(); }
void OrbitRenderer::releaseGraphics()
{
   if (!context() || !scene->context) return;
   makeCurrent(); scene->root->releaseGLObjects(scene->context->getState());
   scene->viewer.setDone(true); scene->viewer.getCamera()->setGraphicsContext(nullptr);
   scene->context=nullptr; doneCurrent();
}
void OrbitRenderer::initializeGL()
{
   GLfloat aliased[2]={1,1},smooth[2]={1,1};
   context()->functions()->glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE,aliased);
   context()->functions()->glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE,smooth);
   scene->nativeLineWidthLimit=std::max(1.0,double(std::min(aliased[1],smooth[1])));
   scene->context=new osgViewer::GraphicsWindowEmbedded(0,0,qRound(width()*devicePixelRatioF()),qRound(height()*devicePixelRatioF()));
   scene->viewer.getCamera()->setGraphicsContext(scene->context);
   scene->viewer.getCamera()->setDrawBuffer(GL_COLOR_ATTACHMENT0);
   scene->viewer.getCamera()->setReadBuffer(GL_COLOR_ATTACHMENT0);
   scene->viewer.setDone(false); scene->viewer.realize();
   connect(context(),&QOpenGLContext::aboutToBeDestroyed,this,[this] { releaseGraphics(); });
}
void OrbitRenderer::resizeGL(int width,int height)
{
   const int w=qRound(width*devicePixelRatioF()),h=qRound(height*devicePixelRatioF());
   if (scene->context) scene->context->resized(0,0,w,h);
   scene->viewer.getCamera()->setViewport(0,0,w,h);
}
void OrbitRenderer::setView(double zoom,double yaw,double pitch,QPointF pan,quint64 frame)
{
   scene->zoom=zoom; scene->yaw=yaw; scene->pitch=pitch; scene->pan=pan; scene->frame=frame; update();
}
void OrbitRenderer::paintGL()
{
   if (!scene->context || width()<=0 || height()<=0) return;
   QPainter nativeGuard(this);
   nativeGuard.beginNativePainting();
   // Qt may leave its own VAO bound. OSG's compatibility arrays must never
   // rewrite it, or later Qt glyphs can use the model's texture coordinates.
   const bool vertexArrays=context()->format().majorVersion()>=3 || context()->hasExtension("GL_ARB_vertex_array_object");
   if (vertexArrays) context()->extraFunctions()->glBindVertexArray(0);
   scene->context->getState()->setCurrentVertexArrayObject(0);
   scene->context->getState()->dirtyAllModes();
   scene->context->getState()->dirtyAllAttributes();
   scene->context->getState()->dirtyAllVertexArrays();
   scene->context->getState()->apply();
   scene->context->setDefaultFboId(defaultFramebufferObject());
   // A Wayland configure or fractional-DPI framebuffer recreation can reach
   // paint/capture after the widget size changed but before resizeGL updated
   // OSG. Derive the viewport from this frame's actual widget dimensions.
   const int framebufferWidth=qRound(width()*devicePixelRatioF()),framebufferHeight=qRound(height()*devicePixelRatioF());
   const auto *traits=scene->context->getTraits();
   if (traits->width!=framebufferWidth || traits->height!=framebufferHeight)
      scene->context->resized(0,0,framebufferWidth,framebufferHeight);
   scene->viewer.getCamera()->setViewport(0,0,framebufferWidth,framebufferHeight);
   scene->synchronize(width(),height(),devicePixelRatioF()); scene->viewer.frame();
   // OSG and Qt share this context. Restore upload/array state before Qt paints
   // its text and controls, including after textured model material traversal.
   auto *state=scene->context->getState();
   if (vertexArrays) context()->extraFunctions()->glBindVertexArray(0);
   state->setCurrentVertexArrayObject(0); state->setCurrentToGlobalVertexArrayState();
   state->disableAllVertexArrays(); state->setActiveTextureUnit(0); state->setClientActiveTextureUnit(0);
   auto *gl=context()->functions();
   gl->glUseProgram(0);
   gl->glBindBuffer(GL_ARRAY_BUFFER,0); gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
   gl->glPixelStorei(GL_UNPACK_ALIGNMENT,4); gl->glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
   gl->glPixelStorei(GL_UNPACK_SKIP_ROWS,0); gl->glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
   if (vertexArrays) context()->extraFunctions()->glBindVertexArray(0);
   gl->glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
   nativeGuard.endNativePainting();
   QImage overlay(QSize(qRound(width()*devicePixelRatioF()),qRound(height()*devicePixelRatioF())),QImage::Format_ARGB32_Premultiplied);
   overlay.setDevicePixelRatio(devicePixelRatioF()); overlay.fill(Qt::transparent);
   { QPainter textPainter(&overlay); drawOverlay(textPainter); }
   nativeGuard.drawImage(QPointF(0,0),overlay);
   nativeGuard.end();
}
QImage OrbitRenderer::captureImage()
{
   auto image=grabFramebuffer();
   image.setDevicePixelRatio(devicePixelRatioF());
   return image;
}
void OrbitRenderer::drawOverlay(QPainter &painter)
{
   painter.setPen(QColor(220,225,235));
   painter.drawText(QRectF(10,height()-28,width()-20,22),Qt::AlignCenter,scene->model->coordinates+" · km");
   if (scene->model->labels) {
      const auto projection=scene->viewer.getCamera()->getViewMatrix()*scene->viewer.getCamera()->getProjectionMatrix();
      for (const auto &curve:scene->model->curves) {
         auto label=[&](const PlotPoint &pose,const QString &text) {
            const auto clip=osg::Vec4d(pose.x,pose.y,pose.z,1)*projection;
            if (clip.w()<=0 || std::abs(clip.x())>clip.w() || std::abs(clip.y())>clip.w() || std::abs(clip.z())>clip.w()) return;
            const osg::Vec3d ndc(clip.x()/clip.w(),clip.y()/clip.w(),clip.z()/clip.w());
            drawOrbitLabel(painter,QPointF((ndc.x()+1)*width()/2,(1-ndc.y())*height()/2),curve,text);
         };
         if (curve.drawsLabel()) if (const auto *pose=orbitObjectPose(curve,scene->frame)) label(*pose,curve.name);
         for (const auto &segment:orbitSegmentLabels(*scene->model,curve,scene->frame)) label(segment.pose,segment.name);
      }
   }
   if (scene->model->labels) {
      const auto projection=scene->viewer.getCamera()->getViewMatrix()*scene->viewer.getCamera()->getProjectionMatrix();
      for (auto it=scene->model->curves.cbegin();it!=scene->model->curves.cend();++it) {
         const auto &curve=it.value(); if (!curve.visible || !curve.objectAxes) continue;
         const auto *pose=orbitObjectPose(curve,scene->frame); if (!pose) continue;
         const auto &prepared=scene->curves.at(it.key());
         const double radius=prepared.modelLoaded ? prepared.modelPose->getBound().radius() : 0;
         const auto axes=orbitObjectAxes(*pose,orbitObjectAxisRadius(curve,radius));
         painter.setPen(pose->color);
         for (int axis=0;axis<3;++axis) {
            const auto &p=axes[axis].end; const auto clip=osg::Vec4d(p.x(),p.y(),p.z(),1)*projection;
            if (clip.w()<=0 || std::abs(clip.x())>clip.w() || std::abs(clip.y())>clip.w() || std::abs(clip.z())>clip.w()) continue;
            painter.drawText(QPointF((clip.x()/clip.w()+1)*width()/2+4,(1-clip.y()/clip.w())*height()/2-4),QString(QChar('X'+axis)));
         }
      }
   }
   if (scene->model->labels) {
      const auto projection=scene->viewer.getCamera()->getViewMatrix()*scene->viewer.getCamera()->getProjectionMatrix();
      for (const auto &vector:scene->model->vectors) if (!vector.label.isEmpty())
         if (const auto arrow=orbitVectorArrow(*scene->model,vector,scene->frame,&scene->vectorBounds)) {
            const auto &point=arrow->end; const auto clip=osg::Vec4d(point.x(),point.y(),point.z(),1)*projection;
            if (clip.w()<=0 || std::abs(clip.x())>clip.w() || std::abs(clip.y())>clip.w() || std::abs(clip.z())>clip.w()) continue;
            painter.setPen(vector.color);
            painter.drawText(QPointF((clip.x()/clip.w()+1)*width()/2+4,(1-clip.y()/clip.w())*height()/2-4),vector.label);
         }
   }
   if (scene->model->legend) {
      int x=12;
      for (const auto &curve:scene->model->curves) {
         if (!curve.drawsContent()) continue;
         painter.setPen(QPen(curve.color,2)); painter.drawLine(x,20,x+15,20);
         painter.setPen(QColor(220,225,235)); painter.drawText(x+20,25,curve.name);
         x+=34+painter.fontMetrics().horizontalAdvance(curve.name);
      }
   }
}
