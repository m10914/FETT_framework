#pragma once

#include <vector>
#include <list>
#include <algorithm>
#include <cmath>


class Vector2
{
public:
    Vector2()
    {
        x = 0;
        y = 0;
        importance = 0;
    }

    Vector2(float _x, float _y, float _importance = 0)
    {
        x = _x;
        y = _y;
        importance = _importance;
    }

    Vector2(const Vector2 &v)
    {
        x = v.x;
        y = v.y;
        importance = v.importance;
    }

    void set(const Vector2 &v)
    {
        x = v.x;
        y = v.y;
        importance = v.importance;
    }

    float dist2(const Vector2 &v)
    {
        float dx = x - v.x;
        float dy = y - v.y;
        return dx * dx + dy * dy;
    }

    float dist(const Vector2 &v)
    {
        return sqrtf(dist2(v));
    }

    float lenSq()
    {
        return x*x + y*y;
    }

    float len()
    {
        return sqrtf(lenSq());
    }

    float dot(const Vector2& vec)
    {
        return x*vec.x + y*vec.y;
    }

    float cross(const Vector2& vec)
    {
        return x*vec.y - y*vec.x;
    }

    float x;
    float y;
    float importance;
};

bool operator == (Vector2 v1, Vector2 v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y);
}

Vector2 operator*(const Vector2 & u, const float & val)
{
    return Vector2(u.x*val, u.y*val);
}

Vector2 operator - (const Vector2 & u, const Vector2 & v)
{
    return Vector2(u.x - v.x, u.y - v.y);
}

Vector2 operator + (const Vector2 & u, const Vector2 & v)
{
    return Vector2(u.x + v.x, u.y + v.y);
}

// signed dist point to segment
float distFromPointToSegment(const Vector2 &src, const Vector2& p0, const Vector2& p1)
{
    Vector2 zero(0, 0);

    Vector2 svec = p1 - p0;
    Vector2 tvec = src - p0;
    float sign = 1;//(svec.cross(tvec) > 0.0f ? 1.0f : -1.0f);

    double l2 = svec.lenSq();
    if(l2 == 0)
    {
        return (src - p0).len() * sign;
    }

    double t = tvec.dot(svec) / l2;
    if(t < 0)
    {
        return tvec.len() * sign;
    }
    else if(t > 1)
    {
        return (src - p1).len() * sign;
    }

    Vector2 proj = p0 + svec * t;
    return (src - proj).len() * sign;

}

class Edge
{
public:
    Edge(const Vector2 &p1, const Vector2 &p2) : p1(p1), p2(p2) {};
    Edge(const Edge &e) : p1(e.p1), p2(e.p2) {};

    Vector2 p1;
    Vector2 p2;
};


inline bool operator == (const Edge & e1, const Edge & e2)
{
    return 	(e1.p1 == e2.p1 && e1.p2 == e2.p2) ||
        (e1.p1 == e2.p2 && e1.p2 == e2.p1);
}


class Triangle
{
public:
    Triangle(const Vector2 &_p1, const Vector2 &_p2, const Vector2 &_p3)
        : p1(_p1), p2(_p2), p3(_p3),
        e1(_p1, _p2), e2(_p2, _p3), e3(_p3, _p1)
    {
    }

    bool containsVertex(const Vector2 &v)
    {
        return p1 == v || p2 == v || p3 == v;
    }

    bool circumCircleContains(const Vector2 &v)
    {
        if(circum_radius < 0.0f)
        {
            float ab = (p1.x * p1.x) + (p1.y * p1.y);
            float cd = (p2.x * p2.x) + (p2.y * p2.y);
            float ef = (p3.x * p3.x) + (p3.y * p3.y);

            circum_x = (ab * (p3.y - p2.y) + cd * (p1.y - p3.y) + ef * (p2.y - p1.y)) / (p1.x * (p3.y - p2.y) + p2.x * (p1.y - p3.y) + p3.x * (p2.y - p1.y)) / 2.f;
            circum_y = (ab * (p3.x - p2.x) + cd * (p1.x - p3.x) + ef * (p2.x - p1.x)) / (p1.y * (p3.x - p2.x) + p2.y * (p1.x - p3.x) + p3.y * (p2.x - p1.x)) / 2.f;
            circum_radius = sqrtf(((p1.x - circum_x) * (p1.x - circum_x)) + ((p1.y - circum_y) * (p1.y - circum_y)));
        }
        float dist = sqrtf(((v.x - circum_x) * (v.x - circum_x)) + ((v.y - circum_y) * (v.y - circum_y)));
        return dist <= circum_radius;
    }

    Vector2 p1;
    Vector2 p2;
    Vector2 p3;
    Edge e1;
    Edge e2;
    Edge e3;

private:
    float circum_x = -1;
    float circum_y = -1;
    float circum_radius = -1;
};

inline bool operator == (const Triangle &t1, const Triangle &t2)
{
    return	(t1.p1 == t2.p1 || t1.p1 == t2.p2 || t1.p1 == t2.p3) &&
        (t1.p2 == t2.p1 || t1.p2 == t2.p2 || t1.p2 == t2.p3) &&
        (t1.p3 == t2.p1 || t1.p3 == t2.p2 || t1.p3 == t2.p3);
}


class TerrainRetriangulator
{
public:
	TerrainRetriangulator(std::vector<Vector2> &vertices, int tstart, int tend)
	{
		// Determinate the super triangle
		float minX = vertices[0].x;
		float minY = vertices[0].y;
		float maxX = minX;
		float maxY = minY;

		for(std::size_t i = tstart; i < tend; ++i)
		{
			if (vertices[i].x < minX) minX = vertices[i].x;
		    if (vertices[i].y < minY) minY = vertices[i].y;
		    if (vertices[i].x > maxX) maxX = vertices[i].x;
		    if (vertices[i].y > maxY) maxY = vertices[i].y;
		}
			
        float dx = maxX - minX;
        float dy = maxY - minY;
        float deltaMax = max(dx, dy);
        float midx = (minX + maxX) / 2.f;
        float midy = (minY + maxY) / 2.f;

        Vector2 p1(midx - 20 * deltaMax, midy - deltaMax);
        Vector2 p2(midx, midy + 20 * deltaMax);
        Vector2 p3(midx + 20 * deltaMax, midy - deltaMax);

        _triangles.push_back(Triangle(p1, p2, p3));

        std::list<Triangle>::iterator t;

        // add vertices with BowyerWatson algorithm
		for(auto p = begin(vertices) + tstart; p != begin(vertices) + tend; p++)
		{
			std::list<Edge> polygon;
			for(t = begin(_triangles); t != end(_triangles);)
			{
				if(t->circumCircleContains(*p))
				{
					polygon.push_back(t->e1);	
					polygon.push_back(t->e2);	
					polygon.push_back(t->e3);
                    _triangles.erase(t++);
				}
                else
                {
                    t++;
                }
			}

			std::vector<Edge> badEdges;
			for(auto e1 = begin(polygon); e1 != end(polygon);)
			{
                bool bFirstDeleted = false;

                auto e2 = e1;
                e2++;
                if(e2 == end(polygon)) break;

				for(; e2 != end(polygon);)
				{
					if(*e1 == *e2)
					{
                        polygon.erase(e2++);
                        bFirstDeleted = true;			
					}
                    else
                    {
                        e2++;
                    }
				}

                if(!bFirstDeleted)
                {
                    e1++;
                }
                else
                {
                    polygon.erase(e1++);
                }
			}

			for(auto e = begin(polygon); e != end(polygon); e++)
            {
                if(
                    (e->p1.x == e->p2.x && e->p2.x == p->x) ||
                    (e->p1.y == e->p2.y && e->p2.y == p->y)
                    )
                    continue;
				_triangles.push_back(Triangle(e->p1, e->p2, *p));
            }
			
		}

		_triangles.erase(std::remove_if(begin(_triangles), end(_triangles), [p1, p2, p3](Triangle &t){
			return t.containsVertex(p1) || t.containsVertex(p2) || t.containsVertex(p3);
		}), end(_triangles));
	}
	
    const std::list<Triangle>& getTriangles() const { return _triangles; };

private:
    std::list<Triangle> _triangles;
	
};
