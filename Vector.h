#pragma once

#include <math.h>


#define _x (0)
#define _y (1)
#define _z (2)
#define _w (3)

#define _right (0)
#define _down (1)
#define _fwd (2)
#define _pos (3)

#define TAU (6.283185307179586)
#define INV_ROOT_2 (0.7071067811865475)


struct Vec3
{
	union
	{
		struct
		{
			double x, y, z;
		};
		double components[3];
	};

	Vec3() {}
	Vec3(int index)
	{
		for(int i = 0; i < 3; i++)
			components[i] = i == index ? 1 : 0;
	}
	Vec3(const Vec3& other)
	{
		for(int i = 0; i < 3; i++)
			components[i] = other[i];
	}
	Vec3(double x, double y, double z = 0)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	inline double operator[] (unsigned int index) const
		{return components[index];}
	inline double& operator[] (unsigned int index)
		{return components[index];}

	inline friend Vec3 operator+ (const Vec3& left, const Vec3& right)
		{return Vec3(left.x + right.x, left.y + right.y, left.z + right.z);}
	inline friend Vec3 operator- (const Vec3& left, const Vec3& right)
		{return Vec3(left.x - right.x, left.y - right.y, left.z - right.z);}
	inline friend Vec3 operator* (const Vec3& left, double right)
		{return Vec3(left.x * right, left.y * right, left.z * right);}
	inline friend Vec3 operator* (double left, const Vec3& right)
		{return Vec3(right.x * left, right.y * left, right.z * left);}
	inline friend Vec3 operator/ (const Vec3& left, double right)
	{
		double temp = 1.0 / right;
		return left * temp;
	}

	inline Vec3 operator- () const
		{return Vec3(-x, -y, -z);}

	//Yup, I'm using * and % for dot and cross product. Sue me.
	inline friend double operator* (const Vec3& left, const Vec3& right)
		{return left.x * right.x + left.y * right.y + left.z * right.z;}
	inline friend Vec3 operator% (const Vec3& left, const Vec3& right)
	{
		return Vec3(
			left.y * right.z - left.z * right.y,
			left.z * right.x - left.x * right.z,
			left.x * right.y - left.y * right.x
		);
	}

	inline double mag2() const
		{return (*this) * (*this);}
	inline double mag() const
		{return sqrt(mag2());}

	inline Vec3 normalize() const
		{return (*this) / mag();}
	inline void normalize_in_place()
	{
		double temp = 1.0 / mag();
		x *= temp;
		y *= temp;
		z *= temp;
	}
};


struct Vec4
{
	union
	{
		struct
		{
			double x, y, z, w;
		};
		double components[4];
	};

	Vec4() {}
	Vec4(int index)
	{
		for(int i = 0; i < 4; i++)
			components[i] = i == index ? 1 : 0;
	}
	Vec4(const Vec3& other, double w = 0)
	{
		for(int i = 0; i < 3; i++)
			components[i] = other[i];
		this->w = w;
	}
	Vec4(const Vec4& other)
	{
		for(int i = 0; i < 4; i++)
			components[i] = other[i];
	}
	Vec4(double x, double y, double z = 0, double w = 0)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	inline double operator[] (unsigned int index) const
		{return components[index];}
	inline double& operator[] (unsigned int index)
		{return components[index];}

	inline friend Vec4 operator+ (const Vec4& left, const Vec4& right)
		{return Vec4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);}
	inline friend Vec4 operator- (const Vec4& left, const Vec4& right)
		{return Vec4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);}
	inline friend Vec4 operator* (const Vec4& left, double right)
		{return Vec4(left.x * right, left.y * right, left.z * right, left.w * right);}
	inline friend Vec4 operator* (double left, const Vec4& right)
		{return Vec4(right.x * left, right.y * left, right.z * left, right.w * left);}
	inline friend Vec4 operator/ (const Vec4& left, double right)
	{
		double temp = 1.0 / right;
		return left * temp;
	}

	inline Vec4 operator- () const
		{return Vec4(-x, -y, -z, -w);}

	inline friend double operator* (const Vec4& left, const Vec4& right)
		{return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;}

	inline double mag2() const
		{return (*this) * (*this);}
	inline double mag() const
		{return sqrt(mag2());}

	inline Vec4 normalize() const
		{return (*this) / mag();}
	inline void normalize_in_place()
	{
		double temp = 1.0 / mag();
		x *= temp;
		y *= temp;
		z *= temp;
		w *= temp;
	}
};


struct Mat4
{
	double data[4][4];		//element i, j is data[i][j]

	Mat4() {}

	Mat4(double fill)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				data[i][j] = fill;
	}

	Mat4(const Mat4& other)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				data[i][j] = other.data[i][j];
	}

	Mat4(const double *components)
	{
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				data[i][j] = components[(i<<2) + j];
	}

	Mat4(const Vec4 right, const Vec4 down, const Vec4 fwd, const Vec4 pos)
	{
		for(int j = 0; j < 4; j++)
		{
			data[_right][j] = right[j];
			data[_down][j] = down[j];
			data[_fwd][j] = fwd[j];
			data[_pos][j] = pos[j];
		}
	}

	Mat4(
		double xx, double xy, double xz, double xw,
		double yx, double yy, double yz, double yw,
		double zx, double zy, double zz, double zw,
		double wx, double wy, double wz, double ww
	) {
		data[_x][_x] = xx;
		data[_x][_y] = xy;
		data[_x][_z] = xz;
		data[_x][_w] = xw;

		data[_y][_x] = yx;
		data[_y][_y] = yy;
		data[_y][_z] = yz;
		data[_y][_w] = yw;

		
		data[_z][_x] = zx;
		data[_z][_y] = zy;
		data[_z][_z] = zz;
		data[_z][_w] = zw;
		
		data[_w][_x] = wx;
		data[_w][_y] = wy;
		data[_w][_z] = wz;
		data[_w][_w] = ww;
	}

	static Mat4 identity()
	{
		Mat4 ret(0.0);
		for(int i = 0; i < 4; i++)
			ret.data[i][i] = 1;
		return ret;
	}

	inline friend Vec4 operator* (const Mat4& left, const Vec4& right)
	{
		Vec4 ret(0, 0, 0, 0);
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				ret[i] += left.data[i][j] * right[j];
		return ret;
	}

	inline friend Vec4 operator* (const Vec4& left, const Mat4& right)
	{
		Vec4 ret(0, 0, 0, 0);
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				ret[j] += left[i] * right.data[i][j];
		return ret;
	}

	inline friend Mat4 operator* (const Mat4& left, const Mat4& right)
	{
		Mat4 ret(0.0);
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				for(int k = 0; k < 4; k++)
					ret.data[i][j] += left.data[i][k] * right.data[k][j];
		return ret;
	}

	inline Mat4 operator~ () const
	{
		Mat4 ret;
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
				ret.data[i][j] = data[i][4 - j];
		return ret;
	}

	inline void set_column(int col, const Vec4& v)
	{
		for(int i = 0; i < 4; i++)
			data[i][col] = v[i];
	}

	inline Vec4 get_column(int col) const
	{
		return Vec4(data[0][col], data[1][col], data[2][col], data[3][col]);
	}

	inline void set_row(int row, const Vec4& v)
	{
		for(int j = 0; j < 4; j++)
			data[row][j] = v[j];
	}

	inline Vec4 get_row(int row) const
	{
		return Vec4(data[row][0], data[row][1], data[row][2], data[row][3]);
	}

	static Mat4 axial_rotation(int ix1, int ix2, double theta)
	{
		Mat4 ret = identity();
		ret.data[ix1][ix1] = ret.data[ix2][ix2] = cos(theta);
		ret.data[ix2][ix1] = -(ret.data[ix1][ix2] = sin(theta));
		return ret;
	}

	inline double determinant() const
	{
		double ret = 0;
		for(int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
			{
				ret += data[(i + j) & 3][j];
				ret -= data[(i - j) & 3][j];
			}
		return ret;
	}
};


void print_vector(const Vec4& v);
void print_matrix(const Mat4& m);
