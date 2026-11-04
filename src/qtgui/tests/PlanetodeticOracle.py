"""Exact spheroid-normal position geometry with GMAT's radial HFPA convention.

Geodetic normal and altitude follow NASA/NAIF georec. Solve altitude from the
specified radius instead of approximating latitude in flattening. Velocity uses
radial/north/east spherical axes as the current GMAT definition does; physical
ellipsoid-horizon convention and Earth orientation truth remain separate gates.
"""
import math


def reference(values,flattening,equatorial_radius):
    radius,longitude,latitude,speed,azimuth,hfpa=values
    if not(radius>equatorial_radius>0 and 0<=flattening<1 and speed>0 and
           -90<=latitude<=90 and -90<=hfpa<=90):
        raise ValueError('Reference scope requires a state outside the reference spheroid')
    lon,lat,az,fpa=map(math.radians,[longitude,latitude,azimuth,hfpa])
    s,c=math.sin(lat),math.cos(lat);e2=flattening*(2-flattening)
    normal_radius=equatorial_radius/math.sqrt(1-e2*s*s);d=e2*normal_radius
    u=d*s*s+math.sqrt(radius*radius-d*d*s*s*c*c)
    horizontal=u*c;z=(u-d)*s;height=u-normal_radius
    position=[horizontal*math.cos(lon),horizontal*math.sin(lon),z]
    center_lat=math.atan2(z,horizontal)
    radial=[math.cos(center_lat)*math.cos(lon),math.cos(center_lat)*math.sin(lon),math.sin(center_lat)]
    north=[-math.sin(center_lat)*math.cos(lon),-math.sin(center_lat)*math.sin(lon),math.cos(center_lat)]
    east=[-math.sin(lon),math.cos(lon),0.]
    velocity=[speed*(math.sin(fpa)*radial[i]+math.cos(fpa)*(math.cos(az)*north[i]+math.sin(az)*east[i])) for i in range(3)]
    return dict(cartesian=position+velocity,altitude=height,geocentric_latitude_deg=math.degrees(center_lat),normal_radius=normal_radius)


def cases():
    return [dict(key=key,representation='Planetodetic',frame='PlanetodeticFixed',flattening=f,equatorial_radius=6378.,values=v,
                 names=['PlanetodeticRMAG','PlanetodeticLON','PlanetodeticLAT','PlanetodeticVMAG','PlanetodeticAZI','PlanetodeticHFPA'],
                 units=['km','deg','deg','km/sec','deg','deg'],invalid_field='PlanetodeticLAT',invalid=['nan','91','-91'],
                 position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,
                 **reference(v,f,6378.)) for key,f,v in [
        ('Sphere',0.,[7300,35,42,7.2,63,12]),
        ('Earthlike',1/300,[7300,125,47,7.5,122,-8]),
        ('Oblate',.1,[9000,215,61,6.8,238,17]),
        ('Southern',.05,[8500,-75,-58,7.1,315,-23])]]


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();args.output.write_text(json.dumps(dict(schema_version=1,position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,cases=cases()),indent=2)+'\n')
