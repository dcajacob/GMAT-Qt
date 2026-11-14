"""Add a mixed-corner POV case to the unchanged visual-transform driver."""
import argparse
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
source = args.source.read_text(encoding='utf-8')
def change(before,after):
    global source
    assert source.count(before)==1,before
    source=source.replace(before,after)
material=r'''mesh2 {
  vertex_vectors { 4, <-20, 0,-10>, <20, 0,-10>, <20, 0,10>, <-20, 0,10> }
  normal_vectors { 1, <0,1,0> }
  texture_list { 2,
    texture { pigment { color rgb <0,1,0> } finish { ambient 1 diffuse 1 } },
    texture { pigment { color rgb <1,0,0> } finish { ambient 1 diffuse 1 } }
  }
  face_indices { 2, <0,1,2>, 0,1,1, <0,2,3>, 0,1,0 }
  normal_indices { 2, <0,0,0>, <0,0,0> }
}
'''
change('  if(key=="Load3DS"){const auto original=', '''  if(key=="LoadPOV"){asset=files.filePath("POV mixed Ω.pov");
   const QByteArray pov=R"GMAT('''+material+''')GMAT";
   write(asset,pov);save("mixed-panel.pov",pov);
  }
  if(key=="Load3DS"){const auto original=''')
change('static Pixels pixels(const QImage &image,bool any)', 'static Pixels pixels(const QImage &image,bool any,bool corner=false)')
change('!(c.green()>50 && c.green()>2*c.red() && c.green()>2*c.blue())', '(corner ? !(std::max(c.green(),c.red())>50 && std::max(c.green(),c.red())>2*c.blue()) : !(c.green()>50 && c.green()>2*c.red() && c.green()>2*c.blue()))')
change('auto p=pixels(image,arbitrary);QJsonObject record', '''const bool corner=c["asset"].toString()=="pov";auto p=pixels(image,arbitrary,corner);QJsonObject record''')
change('   if(arbitrary)require(p.count>1000', '''   if(corner) {
      int red=0, green=0;
      for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
         const auto c=image.pixelColor(x,y);
         red += c.red()>50 && c.red()>2*c.green() && c.red()>2*c.blue();
         green += c.green()>50 && c.green()>2*c.red() && c.green()>2*c.blue();
      }
      record["red_corner_pixels"]=red;record["green_corner_pixels"]=green;
      require(red>50 && green>50,"Mixed POV corner materials did not render both endpoint colors");
   }
   if(arbitrary)require(p.count>1000''')
change('auto model=visual->previewModel();require(model', '''if(key=="LoadPOV"){
    require(QTest::qWaitFor([&]{auto m=visual->previewModel();auto *status=visual->findChild<QLabel*>("visualModelError");
       return m && !m->curves.isEmpty() && m->curves[0].modelPath==asset && status && status->text().isEmpty();},10000),
       "Asynchronous POV preview did not accept the selected model");
   }auto model=visual->previewModel();require(model''')
change('acceptanceView(*d);if(key=="Load3DS")', '''acceptanceView(*d);if(key=="LoadPOV")require(d->nameFilters().join(" ").contains("*.pov"),"POV absent from actual chooser filter");if(key=="Load3DS" || key=="LoadPOV")''')
change('analytic OBJ or one shipped3DS asset.', 'Analytic mixed-corner POV model.')
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(source, encoding='utf-8')
