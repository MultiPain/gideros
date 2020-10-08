#include "lua.hpp"
#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>

struct VEC {
	double x;
	double y;
	double z;
};

enum VECTYPE {
	VT_ARGS,
	VT_ARGS3,
	VT_ARRAY,
	VT_ARRAY3,
	VT_TABLE,
	VT_TABLE3
};

namespace GidMath {

class Shape {
public:
	struct Hit {
		double distance;
		VEC point;
		VEC normal;
		VEC reflect;
	};
protected:
	void addHit(VEC o,VEC d,std::vector<Hit> &hits,double l);
public:
	virtual ~Shape() {};
	virtual double inside(VEC p)=0;
	virtual VEC nearest(VEC p)=0;
	virtual void raycast(VEC o,VEC d,std::vector<Hit> &hits)=0;
	virtual void computeNormal(Hit &h)=0;
};

void Shape::addHit(VEC o,VEC d,std::vector<Hit> &hits,double l)
{
    if (l<0) return;
	Hit h;
	h.distance=l;
	h.point.x=o.x+d.x*l;
	h.point.y=o.y+d.y*l;
	h.point.z=o.z+d.z*l;
	computeNormal(h);
	hits.push_back(h);
}

class ShapeGroup : public Shape {
	std::vector<Shape *> shapes;
	VEC origin;
public:
	ShapeGroup(VEC o) {
		origin=o;
	}
	~ShapeGroup() {
		for (std::vector<Shape *>::iterator it=shapes.begin();it!=shapes.end();it++)
			delete (*it);
	}
	void add(Shape *s) { shapes.push_back(s); }
    virtual void computeNormal(Hit &) { };
	double inside(VEC p) {
		p.x-=origin.x;
		p.y-=origin.y;
		p.z-=origin.z;
		double dd=DBL_MAX;
		for (std::vector<Shape *>::iterator it=shapes.begin();it!=shapes.end();it++) {
            double dist=(*it)->inside(p);
			if (dist<dd) dd=dist;
		}
		return dd;
	}
	VEC nearest(VEC p) {
		VEC v;
		p.x-=origin.x;
		p.y-=origin.y;
		p.z-=origin.z;
		double dd=DBL_MAX;
		for (std::vector<Shape *>::iterator it=shapes.begin();it!=shapes.end();it++) {
            VEC vn=(*it)->nearest(p);
			double dist=(vn.x-p.x)*(vn.x-p.x)+(vn.y-p.y)*(vn.y-p.y)+(vn.z-p.z)*(vn.z-p.z);
			if (dist<dd) { v=vn; dd=dist; }
		}
		v.x+=origin.x;
		v.y+=origin.y;
		v.z+=origin.z;
		return v;
	}
    void raycast(VEC o,VEC d,std::vector<Hit> &hits) {
    	std::vector<Shape::Hit> lhits;
        o.x-=origin.x;
        o.y-=origin.y;
        o.z-=origin.z;
		for (std::vector<Shape *>::iterator it=shapes.begin();it!=shapes.end();it++)
            (*it)->raycast(o,d,lhits);
   		for (std::vector<Shape::Hit>::iterator it=lhits.begin();it!=lhits.end();it++) {
   			it->point.x+=origin.x;
   			it->point.y+=origin.y;
   			it->point.z+=origin.z;
   			hits.push_back(*it);
   		}
	}
};

class ShapeSphere : public Shape {
	VEC center;
	double radius;
public:
	ShapeSphere(VEC c,double r) {
		center=c;
		radius=r;
	}
	double inside(VEC p) {
		VEC v;
		v.x=p.x-center.x;
		v.y=p.y-center.y;
		v.z=p.z-center.z;
		double d=sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
		return d-radius;
	}
	VEC nearest(VEC p) {
		VEC v;
		v.x=p.x-center.x;
		v.y=p.y-center.y;
		v.z=p.z-center.z;
		double d=radius/sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
		v.x=center.x+v.x*d;
		v.y=center.y+v.y*d;
		v.z=center.z+v.z*d;
		return v;
	}
    void raycast(VEC o,VEC d,std::vector<Hit> &hits) {
		double a=d.x*d.x+d.y*d.y+d.z*d.z;
        if (a<0) return;
        double b=2*((o.x-center.x)*d.x+(o.y-center.y)*d.y+(o.z-center.z)*d.z);
        double c=o.x*(o.x-2*center.x)+o.y*(o.y-2*center.y)+o.z*(o.z-2*center.z)-radius*radius
                +center.x*center.x+center.y*center.y+center.z*center.z;
		double delta=b*b-4*a*c;
		if (delta>0)
		{
			double sd=sqrt(delta);
            addHit(o,d,hits,(-b-sd)/(2*a));
            addHit(o,d,hits,(-b+sd)/(2*a));
		}
		else {
			if (delta==0)
                addHit(o,d,hits,-b/(2*a));
		}
	}

	void computeNormal(Hit &h) {
		VEC n;
		n.x=h.point.x-center.x;
		n.y=h.point.y-center.y;
		n.z=h.point.z-center.z;
		double nl=sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
		h.normal.x=n.x/nl;
		h.normal.y=n.y/nl;
		h.normal.z=n.z/nl;
	}
};

class ShapePlane : public Shape {
	VEC center;
	VEC normal;
	double extent;
public:
	ShapePlane(VEC c,VEC n,double e) {
		center=c;
		normal=n;
		extent=e;
	}
	double inside(VEC p) {
		double d;
		d=(p.x-center.x)*normal.x;
		d+=(p.y-center.y)*normal.y;
		d+=(p.z-center.z)*normal.z;
		return d;
	}
	VEC nearest(VEC p) {
		VEC v;
		v.x=p.x-center.x;
		v.y=p.y-center.y;
		v.z=p.z-center.z;
		double d;
		d=v.x*normal.x;
		d+=v.y*normal.y;
		d+=v.z*normal.z;
		v.x=p.x-normal.x*d;
        v.y=p.y-normal.y*d;
        v.z=p.z-normal.z*d;
		if (extent) {
			VEC r;
			r.x=v.x-center.x;
			r.y=v.y-center.y;
			r.z=v.z-center.z;
			double d=sqrt(r.x*r.x+r.y*r.y+r.z*r.z);
			if (d>extent) {
				double s=extent/d;
				v.x=center.x+r.x*s;
				v.y=center.y+r.y*s;
				v.z=center.z+r.z*s;
			}
		}
		return v;
	}
	void raycast(VEC o,VEC d,std::vector<Hit> &hits) {
		double dn;
		dn=d.x*normal.x;
		dn+=d.y*normal.y;
		dn+=d.z*normal.z;
		if (dn) {
			double on;
			on=(center.x-o.x)*normal.x;
			on+=(center.y-o.y)*normal.y;
			on+=(center.z-o.z)*normal.z;
            on/=dn;
			if (extent) {
				VEC r;
				r.x=o.x+d.x*on-center.x;
				r.y=o.y+d.y*on-center.y;
				r.z=o.z+d.z*on-center.z;
				double rd=sqrt(r.x*r.x+r.y*r.y+r.z*r.z);
				if (rd<=extent)
                    addHit(o,d,hits,on);
			}
			else
				addHit(o,d,hits,on/dn);
		}
	}

	void computeNormal(Hit &h) {
		h.normal=normal;
	}
};

#define abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
                                        lua_gettop(L) + (i) + 1)

static VECTYPE probeVec(lua_State *L,int idx,int d3) {
    idx=abs_index(L,idx);
	if (lua_type(L,idx)==LUA_TTABLE) {
	  	  lua_getfield(L,idx,"x");
	  	  bool arr=lua_isnil(L,-1);
	  	  lua_getfield(L,idx,"z");
	  	  bool arr3=lua_isnil(L,-1);
	  	  lua_pop(L,2);
	  	  if (!arr3) return VT_TABLE3;
	  	  if (!arr) return VT_TABLE;
	  	  return (lua_objlen(L,idx)>2)?VT_ARRAY3:VT_ARRAY;
  	}
  	return lua_isnoneornil(L,d3)?VT_ARGS:VT_ARGS3;
}

static int getVec(lua_State *L,int idx,VECTYPE vt,VEC &v) {
    idx=abs_index(L,idx);
    if ((vt==VT_ARGS)||(vt==VT_ARGS3)) {
		v.x=luaL_checknumber(L,idx);
		v.y=luaL_checknumber(L,idx+1);
		v.z=(vt==VT_ARGS3)?luaL_checknumber(L,idx+2):0;
		return idx+((vt==VT_ARGS3)?3:2);
	}
	else {
		luaL_checktype(L,idx,LUA_TTABLE);
		if ((vt==VT_ARRAY)||(vt==VT_ARRAY3)) {
			lua_rawgeti(L,idx,1);
			lua_rawgeti(L,idx,2);
			lua_rawgeti(L,idx,3);
		}
		else {
			lua_getfield(L,idx,"x");
			lua_getfield(L,idx,"y");
			lua_getfield(L,idx,"z");
		}
        v.x=luaL_optnumber(L,-3,0);
        v.y=luaL_optnumber(L,-2,0);
		v.z=luaL_optnumber(L,-1,0);
		lua_pop(L,3);
		return idx+1;
	}
}

static int pushVec(lua_State *L,VECTYPE vt,VEC v) {
	if ((vt==VT_ARGS)||(vt==VT_ARGS3)) {
		lua_pushnumber(L,v.x);
		lua_pushnumber(L,v.y);
		if (vt==VT_ARGS) return 2;
		lua_pushnumber(L,v.z);
		return 3;
	}
	else {
		if ((vt==VT_ARRAY)||(vt==VT_ARRAY3)) {
			lua_createtable(L,3,0);
			lua_pushnumber(L,v.x);
			lua_rawseti(L,-2,1);
			lua_pushnumber(L,v.y);
			lua_rawseti(L,-2,2);
			if (vt==VT_ARRAY) return 1;
			lua_pushnumber(L,v.z);
			lua_rawseti(L,-2,3);
		}
		else {
			lua_createtable(L,0,3);
			lua_pushnumber(L,v.x);
			lua_setfield(L,-2,"x");
			lua_pushnumber(L,v.y);
			lua_setfield(L,-2,"y");
			if (vt==VT_TABLE) return 1;
			lua_pushnumber(L,v.z);
			lua_setfield(L,-2,"z");
		}
		return 1;
	}
}

static Shape *getShape(lua_State *L,int idx,int ti) {
    idx=abs_index(L,idx);
    if (lua_type(L,idx)!=LUA_TTABLE)
	{
		if (ti)
			lua_pushstring(L,"Shape definition must be a table");
		else
			lua_pushfstring(L,"Shape definition must be a table (at index %d)",ti);
		lua_error(L);
	}
	//Test for sphere
	lua_getfield(L,idx,"radius");
	if (!lua_isnil(L,-1)) {
		double radius=luaL_optnumber(L,-1,1);
		lua_pop(L,1);
		VEC ctr;
		getVec(L,idx,VT_TABLE3,ctr);
		return new ShapeSphere(ctr,radius);
	}
	lua_pop(L,1);
	//Test for plane
	lua_getfield(L,idx,"normal");
	if (!lua_isnil(L,-1)) {
		VEC n;
		getVec(L,-1,VT_TABLE3,n);
		lua_getfield(L,idx,"extent");
		double extent=luaL_optnumber(L,-1,0);
		lua_pop(L,2);
		VEC ctr;
		getVec(L,idx,VT_TABLE3,ctr);
		return new ShapePlane(ctr,n,extent);
	}
	lua_pop(L,1);
    //Check for a 2D line
    lua_getfield(L,idx,"dx");
    if (!lua_isnil(L,-1)) {
        double dx=luaL_optnumber(L,-1,0);
        lua_getfield(L,idx,"dy");
        double dy=luaL_optnumber(L,-1,0);
        lua_pop(L,2);
        VEC ctr,n;
        getVec(L,idx,VT_TABLE3,ctr);
        n.x=dy;
        n.y=-dx;
        n.z=0;
        return new ShapePlane(ctr,n,0);
    }
    lua_pop(L,1);
    //Check for a 2D segment
    lua_getfield(L,idx,"x2");
    if (!lua_isnil(L,-1)) {
        double x2=luaL_optnumber(L,-1,0);
        lua_getfield(L,idx,"y2");
        double y2=luaL_optnumber(L,-1,0);
        lua_pop(L,2);
        VEC ctr,n;
        getVec(L,idx,VT_TABLE3,ctr);
        double dx=x2-ctr.x;
        double dy=y2-ctr.y;
        ctr.x+=dx/2;
        ctr.y+=dy/2;
        double dn=sqrt(dx*dx+dy*dy);
        n.x=dy/dn;
        n.y=-dx/dn;
        n.z=0;
        return new ShapePlane(ctr,n,dn/2);
    }
    lua_pop(L,1);

    //Not a specific shape, assume a group
    VEC origin;
    getVec(L,idx,VT_TABLE3,origin);
    ShapeGroup *group=new ShapeGroup(origin);
	int ln=lua_objlen(L,idx);
	for (int in=1;in<=ln;in++) {
		lua_rawgeti(L,idx,in);
		Shape *s=getShape(L,-1,in);
		lua_pop(L,1);
		group->add(s);
	}
	return group;
}

}

using namespace GidMath;
static int math_length (lua_State *L) {
	VEC v;
	VECTYPE vt=probeVec(L,1,3);
	getVec(L,1,vt,v);
	lua_pushnumber(L, sqrt(v.x*v.x+v.y*v.y+v.z*v.z));
	return 1;
}

static int math_distance (lua_State *L) {
    VEC v1,v;
    VECTYPE vt=probeVec(L,1,5);
    int idx=getVec(L,1,vt,v1);
    getVec(L,idx,vt,v);
    v.x-=v1.x; v.y-=v1.y; v.z-=v1.z;
    lua_pushnumber(L, sqrt(v.x*v.x+v.y*v.y+v.z*v.z));
    return 1;
}

static int math_cross (lua_State *L) {
    VEC u,v,c;
    VECTYPE vt=probeVec(L,1,5);
    int idx=getVec(L,1,vt,u);
    getVec(L,idx,vt,v);
    c.x=u.y*v.z-u.z*v.y;
    c.y=u.z*v.x-u.x*v.z;
    c.z=u.x*v.y-u.y*v.x;
    if (vt==VT_ARGS) vt=VT_ARGS3;
    if (vt==VT_ARRAY) vt=VT_ARRAY3;
    if (vt==VT_TABLE) vt=VT_TABLE3;
    pushVec(L,vt,c);
    return 1;
}

static int math_dot (lua_State *L) {
    VEC v1,v;
    VECTYPE vt=probeVec(L,1,5);
    int idx=getVec(L,1,vt,v1);
    getVec(L,idx,vt,v);
    lua_pushnumber(L, v.x*v1.x+v.y*v1.y+v.z*v1.z);
    return 1;
}

static bool dist_sorta(std::pair<int,double> a,std::pair<int,double> b) { return (a.second<b.second); }
static bool dist_sortb(std::pair<int,double> a,std::pair<int,double> b) { return (b.second<a.second); }
static bool hit_sort(Shape::Hit a,Shape::Hit b) { return (a.distance<b.distance); }

static int math_distances (lua_State *L) {
	VEC v1,v;
	luaL_checktype(L,1,LUA_TTABLE);
	luaL_checktype(L,2,LUA_TTABLE);
	int sort=luaL_optinteger(L,3,0);
	VECTYPE vt=probeVec(L,1,1);
	getVec(L,1,vt,v1);

	int ln=lua_objlen(L,2);
	lua_createtable(L,ln,0);
	std::vector<std::pair<int,double>> dists;
	for (int in=1;in<=ln;in++) {
		lua_rawgeti(L,2,in);
		getVec(L,-1,vt,v);
		lua_pop(L,1);
		v.x-=v1.x; v.y-=v1.y; v.z-=v1.z;
		double dist=sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
		if (sort)
			dists.push_back(std::pair<int,double>(in,dist));
		else {
			lua_pushnumber(L, dist);
			lua_rawseti(L,-2,in);
		}
	}
	if (sort) {
		std::sort(dists.begin(),dists.end(),(sort>0)?dist_sorta:dist_sortb);
		for (int in=1;in<=ln;in++) {
			lua_createtable(L,2,0);
            lua_rawgeti(L,2,dists[in-1].first);
			lua_rawseti(L,-2,1);
            lua_pushnumber(L, dists[in-1].second);
			lua_rawseti(L,-2,2);
			lua_rawseti(L,-2,in);
		}
	}
	return 1;
}

static int math_nearest (lua_State *L) {
	VEC v1,v;
	luaL_checktype(L,1,LUA_TTABLE);
	luaL_checktype(L,2,LUA_TTABLE);
	VECTYPE vt=probeVec(L,1,1);
	getVec(L,1,vt,v1);

	int ln=lua_objlen(L,2);
	int nr=0;
	double dd=DBL_MAX;
	for (int in=1;in<=ln;in++) {
		lua_rawgeti(L,2,in);
		getVec(L,-1,vt,v);
		lua_pop(L,1);
		v.x-=v1.x; v.y-=v1.y; v.z-=v1.z;
		double dist=sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
		if (dist<dd) {
			nr=in;
			dd=dist;
		}
	}
	if (nr) {
		lua_rawgeti(L,2,nr);
		lua_pushnumber(L,dd);
		return 2;
	}
	return 0;
}

static int math_normalize (lua_State *L) {
	VEC v;
	VECTYPE vt=probeVec(L,1,3);
	getVec(L,1,vt,v);
	double l=sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
	v.x/=l;
	v.y/=l;
	v.z/=l;
	return pushVec(L,vt,v);
}

static int math_raycast (lua_State *L) {
	VEC vo,vn;
	VECTYPE vt=probeVec(L,1,5);
    int optpos=4;
    if (vt==VT_ARGS) optpos=6;
    if (vt==VT_ARGS3) optpos=8;
    int limit=luaL_optnumber(L,optpos,0);
    int idx=getVec(L,1,vt,vo);
	idx=getVec(L,idx,vt,vn);
	std::vector<Shape::Hit> hits;
	Shape *s=getShape(L,idx,0);
    s->raycast(vo, vn, hits);
	delete s;
    std::sort(hits.begin(),hits.end(),hit_sort);
    lua_createtable(L,hits.size(),0);
	if (hits.size()) {
		int hn=0;
		if (vt==VT_ARGS) vt=VT_TABLE;
		if (vt==VT_ARGS3) vt=VT_TABLE3;
		for (std::vector<Shape::Hit>::iterator it=hits.begin();it!=hits.end();it++) {
			lua_createtable(L,0,4);
			lua_pushnumber(L,it->distance);
			lua_setfield(L,-2,"distance");
			pushVec(L,vt,it->point);
			lua_setfield(L,-2,"point");
			pushVec(L,vt,it->normal);
			lua_setfield(L,-2,"normal");
            double dn=2*(vn.x*it->normal.x+vn.y*it->normal.y+vn.z*it->normal.z);
            it->reflect.x=vn.x-dn*it->normal.x;
            it->reflect.y=vn.y-dn*it->normal.y;
            it->reflect.z=vn.z-dn*it->normal.z;
			pushVec(L,vt,it->reflect);
			lua_setfield(L,-2,"reflect");
			lua_rawseti(L,-2,++hn);
            if (hn==limit) break;
		}
	}
	return 1;
}

static int math_inside (lua_State *L) {
	VEC v;
	VECTYPE vt=probeVec(L,1,3);
	int idx=getVec(L,1,vt,v);
	Shape *s=getShape(L,idx,0);
	lua_pushnumber(L,s->inside(v));
	delete s;
	return 1;
}

static int math_edge (lua_State *L) {
	VEC v;
	VECTYPE vt=probeVec(L,1,3);
	int idx=getVec(L,1,vt,v);
	Shape *s=getShape(L,idx,0);
	int rs=pushVec(L,vt,s->nearest(v));
	delete s;
	return rs;
}

void register_gideros_math(lua_State *L) {
	// Register math helpers
	static const luaL_Reg mathlib[] = {
      {"length",   math_length},
      {"cross",   math_cross},
      {"dot",   math_dot},
      {"distance",  math_distance},
	  {"distances",  math_distances},
	  {"nearest",  math_nearest},
	  {"normalize",  math_normalize},
	  {"raycast", math_raycast},
	  {"inside", math_inside},
	  {"edge", math_edge},
	  {NULL, NULL}
	};
	luaL_register(L, LUA_MATHLIBNAME, mathlib);
}