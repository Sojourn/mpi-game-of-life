#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>
#include <cstddef>
#include <cassert>

// A dynamic 2d array.
template<class T>
class Array2D
{
public:

	// Default constructor creates an invalid array. Call resize before use.
	inline Array2D();

	// Construct a 2d array with the given width and height.
	inline Array2D(size_t width, size_t height);

	// Copy construct from anouther 2d array.
	inline Array2D(const Array2D<T> &other);

	// Copy from anouther array. May resize this array.
	inline const Array2D &operator=(const Array2D &other);

	// Dtor.
	inline ~Array2D(void);

	// Destructively resize the 2d array.
	inline void resize(size_t width, size_t height);

	// Return the number of columns.
	inline size_t width() const;

	// Return the number of rows.
	inline size_t height() const;
	
	// Return a pointer to the data store.
	inline T** data();

	// Return a reference to the element at the location. Bounds checked.
	inline T& at(size_t x, size_t y);

	// Return a const reference to the element at the location. Bounds checked.
	inline const T& at(size_t x, size_t y) const;

	// Return a pointer to the y'th row. No bounds checking.
	inline T* operator[](size_t y);

	// Return a const pointer to the y'th row. No bounds checking.
	inline const T* operator[](size_t y) const;

private:

	// Frees resources and invalidates the array.
	void release();

	size_t _width;
	size_t _height;
	T** _array;
};

template<class T>
Array2D<T>::Array2D() :
	_width(0),
	_height(0),
	_array(NULL)
{
}

template<class T>
Array2D<T>::Array2D(size_t width, size_t height) :
	_width(0),
	_height(0),
	_array(NULL)
{
	resize(width, height);
}

template<class T>
Array2D<T>::Array2D(const Array2D<T> &other) :
	_width(0),
	_height(0),
	_array(NULL)
{
	if((other.width() != width()) || (other.height() != height()))
	{
		resize(other.width(), other.height());
	}

	for(size_t y = 0; y < height(); y++)
	{
		for(size_t x = 0; x < width(); x++)
		{
			at(x, y) = other.at(x, y);
		}
	}
}

template<class T>
const Array2D<T> &Array2D<T>::operator=(const Array2D<T> &other)
{
	if(this != &other)
	{
		if((other.width() != width()) || (other.height() != height()))
		{
			resize(other.width(), other.height());
		}

		for(size_t y = 0; y < height(); y++)
		{
			for(size_t x = 0; x < width(); x++)
			{
				at(x, y) = other.at(x, y);
			}
		}
	}

	return *this;
}

template<class T>
Array2D<T>::~Array2D(void)
{
	release();
}

template<class T>
void Array2D<T>::resize(size_t width, size_t height)
{
	assert(width > 0);
	assert(height > 0);

	release();

	_width = width;
	_height = height;

	_array = new T*[sizeof(T*) * _height];

	T* elements = new T[width * height];
	for(size_t y = 0; y < _height; y++)
	{
		_array[y] = &elements[y * _width]; 
	}
}

template<class T>
size_t Array2D<T>::width() const
{
	return _width;
}

template<class T>
size_t Array2D<T>::height() const
{
	return _height;
}

template<class T>
T** Array2D<T>::data()
{
	return _array;
}

template<class T>
T& Array2D<T>::at(size_t x, size_t y)
{
	assert(x < width());
	assert(y < height());
	assert(_array != NULL);
	return (*this)[y][x];
}

template<class T>
const T& Array2D<T>::at(size_t x, size_t y) const
{
	assert(x < width());
	assert(y < height());
	assert(_array != NULL);
	return (*this)[y][x];
}

template<class T>
T* Array2D<T>::operator[](size_t y)
{
	return _array[y];
}

template<class T>
const T* Array2D<T>::operator[](size_t y) const
{
	return _array[y];
}

template<class T>
void Array2D<T>::release()
{
	if(data() != NULL)
	{
		delete [] _array[0];
		delete [] _array;
	}

	_width = 0;
	_height = 0;
	_array = NULL;
}

#endif // ARRAY_H
