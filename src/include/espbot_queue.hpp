/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __QUEUE_HPP__
#define __QUEUE_HPP__

typedef enum
{
  Queue_ok = 0,
  Queue_full = -1,
  Queue_empty = -2
} Queue_err;

template <class T>
class Queue
{
public:
  Queue(int max_size);
  ~Queue();
  bool empty();
  bool full();
  int size();
  T *front();
  T *next();
  Queue_err push(T *);
  Queue_err pop();

private:
  int _max_size;
  int _size;
  T **_content;
  int _front;
  int _back;
  int _cursor;
};

template <class T>
Queue<T>::Queue(int max_size)
{
  _max_size = max_size;
  _size = 0;
  _front = 0;
  _back = 0;
  _content = new T *[_max_size];
  int idx;
  // if (_content == NULL)
  // {
  //   PRINT_ERROR("Queue - not enough heap memory [%d]\n", max_size);
  // }
}

template <class T>
Queue<T>::~Queue()
{
  if (_content)
    delete[] _content;
}

template <class T>
bool Queue<T>::empty()
{
  if (_size == 0)
    return true;
  else
    return false;
}

template <class T>
bool Queue<T>::full()
{
  if (_size == _max_size)
    return true;
  else
    return false;
}

template <class T>
int Queue<T>::size()
{
  return _size;
}

template <class T>
T *Queue<T>::front()
{
  if (_size)
  {
    _cursor = _front;
    return _content[_cursor];
  }
  else
    return NULL;
}

template <class T>
T *Queue<T>::next()
{
  _cursor++;
  if (_cursor == _max_size)
    _cursor = 0;
  if (_cursor == _front)
    return NULL;
  if (_content[_cursor])
    return _content[_cursor];
  else
    return NULL;
}

template <class T>
Queue_err Queue<T>::push(T *elem)
{
  if ((_size == _max_size) || (_content == NULL))
    return Queue_full;
  if (_size > 0)
  {
    _back++;
    if (_back == _max_size)
      _back = 0;
  }
  _size++;
  _content[_back] = elem;
  return Queue_ok;
}

template <class T>
Queue_err Queue<T>::pop()
{
  if (_size == 0)
    return Queue_empty;
  _size--;
  _content[_front] = 0;
  _front++;
  if (_front == _max_size)
    _front = 0;
  if (_size == 0)
    _back = _front;
  return Queue_ok;
}

#endif