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

#ifdef ESPBOT

#define PRINT_FATAL(...) esplog.fatal(__VA_ARGS__)
#define PRINT_ERROR(...) esplog.error(__VA_ARGS__)
#define PRINT_WARN(...) esplog.warn(__VA_ARGS__)
#define PRINT_INFO(...) esplog.info(__VA_ARGS__)
#define PRINT_DEBUG(...) esplog.debug(__VA_ARGS__)
#define PRINT_TRACE(...) esplog.trace(__VA_ARGS__)
#define PRINT_ALL(...) esplog.all(__VA_ARGS__)

#else

#define PRINT_FATAL(...) printf(__VA_ARGS__)
#define PRINT_ERROR(...) printf(__VA_ARGS__)
#define PRINT_WARN(...) printf(__VA_ARGS__)
#define PRINT_INFO(...) printf(__VA_ARGS__)
#define PRINT_DEBUG(...) printf(__VA_ARGS__)
#define PRINT_TRACE(...) printf(__VA_ARGS__)
#define PRINT_ALL(...) printf(__VA_ARGS__)

#endif

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
  T *back();
  Queue_err push(T *);
  Queue_err pop();

private:
  int m_max_size;
  int m_size;
  T **m_content;
  int m_front;
  int m_back;
};

template <class T>
Queue<T>::Queue(int max_size)
{
  m_max_size = max_size;
  m_size = 0;
  m_front = 0;
  m_back = 0;
  m_content = new T *[m_max_size];
  // if (m_content == NULL)
  // {
  //   PRINT_ERROR("Queue - not enough heap memory [%d]\n", max_size);
  // }
}

template <class T>
Queue<T>::~Queue()
{
  if (m_content)
    delete[] m_content;
}

template <class T>
bool Queue<T>::empty()
{
  if (m_size == 0)
    return true;
  else
    return false;
}

template <class T>
bool Queue<T>::full()
{
  if (m_size == m_max_size)
    return true;
  else
    return false;
}

template <class T>
int Queue<T>::size()
{
  return m_size;
}

template <class T>
T *Queue<T>::front()
{
  if (m_size)
    return m_content[m_front];
  else
    return NULL;
}

template <class T>
T *Queue<T>::back()
{
  if (m_size)
    return m_content[m_back];
  else
    return NULL;
}

template <class T>
Queue_err Queue<T>::push(T *elem)
{
  if ((m_size == m_max_size) || (m_content == NULL))
    return Queue_full;
  if (m_size > 0)
  {
    m_back++;
    if (m_back == m_max_size)
      m_back = 0;
  }
  m_size++;
  m_content[m_back] = elem;
  return Queue_ok;
}

template <class T>
Queue_err Queue<T>::pop()
{
  if (m_size == 0)
    return Queue_empty;
  m_size--;
  m_front++;
  if (m_front == m_max_size)
    m_front = 0;
  if (m_size == 0)
    m_back = m_front;
  return Queue_ok;
}

#endif