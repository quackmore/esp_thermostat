/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __List_HPP__
#define __List_HPP__

typedef enum
{
  list_ok = 0,
  list_full = -1,
  list_empty = -2,
  list_alloc_fail = -3
} List_err;

typedef enum
{
  dont_delete_content = 0,
  delete_content,
  dont_ovverride_when_full,
  override_when_full
} List_param;

template <class T>
class List
{
public:
  List(int max_size, List_param del = dont_delete_content);
  ~List();
  bool empty();
  bool full();
  int size();
  T *front();
  T *back();
  T *next();
  T *prev();
  List_err push_front(T *, List_param ovr = dont_ovverride_when_full);
  List_err push_back(T *, List_param ovr = dont_ovverride_when_full);
  List_err remove();
  List_err pop_front();
  List_err pop_back();

private:
  struct list_el
  {
    T *content;
    struct list_el *next;
    struct list_el *prev;
  };
  int m_max_size;
  int m_size;
  struct list_el *m_front;
  struct list_el *m_back;
  struct list_el *m_cursor;
  List_param m_delete_content;
};

template <class T>
List<T>::List(int max_size, List_param del)
{
  m_max_size = max_size;
  m_delete_content = del;
  m_size = 0;
  m_front = NULL;
  m_back = NULL;
}

template <class T>
List<T>::~List()
{
  struct list_el *ptr, *tmp_ptr;
  ptr = m_front;
  while (ptr)
  {
    if (m_delete_content == delete_content)
      delete ptr->content;
    tmp_ptr = ptr;
    ptr = ptr->next;
    delete tmp_ptr;
  }
}

template <class T>
bool List<T>::empty()
{
  if (m_size == 0)
    return true;
  else
    return false;
}

template <class T>
bool List<T>::full()
{
  if (m_size == m_max_size)
    return true;
  else
    return false;
}

template <class T>
int List<T>::size()
{
  return m_size;
}

template <class T>
T *List<T>::front()
{
  m_cursor = m_front;
  if (m_front)
    return m_front->content;
  else
    return NULL;
}

template <class T>
T *List<T>::back()
{
  m_cursor = m_back;
  if (m_back)
    return m_back->content;
  else
    return NULL;
}

template <class T>
T *List<T>::next()
{
  if (m_cursor)
  {
    if (m_cursor->next)
    {
      m_cursor = m_cursor->next;
      return m_cursor->content;
    }
  }
  return NULL;
}

template <class T>
T *List<T>::prev()
{
  if (m_cursor)
  {
    if (m_cursor->prev)
    {
      m_cursor = m_cursor->prev;
      return m_cursor->content;
    }
  }
  return NULL;
}

template <class T>
List_err List<T>::push_front(T *elem, List_param ovr)
{
  struct list_el *el_ptr = new struct list_el;
  if (el_ptr)
  {
    if (m_size == m_max_size)
    {
      if (ovr == dont_ovverride_when_full)
        return list_full;
      else
        pop_back();
    }
    m_size++;
    el_ptr->next = m_front;
    el_ptr->prev = NULL;
    el_ptr->content = elem;

    if (m_front)
      m_front->prev = el_ptr;
    m_front = el_ptr;

    if (m_back == NULL)
      m_back = el_ptr;
    return list_ok;
  }
  else
  {
    return list_alloc_fail;
  }
}

template <class T>
List_err List<T>::push_back(T *elem, List_param ovr)
{
  struct list_el *el_ptr = new struct list_el;
  if (el_ptr)
  {
    if (m_size == m_max_size)
    {
      if (ovr == dont_ovverride_when_full)
        return list_full;
      else
        pop_front();
    }
    m_size++;
    el_ptr->next = NULL;
    el_ptr->prev = m_back;
    el_ptr->content = elem;

    if (m_back)
      m_back->next = el_ptr;
    m_back = el_ptr;

    if (m_front == NULL)
      m_front = el_ptr;
    return list_ok;
  }
  else
  {
    return list_alloc_fail;
  }
}

template <class T>
List_err List<T>::remove()
{
  if (m_cursor)
  {
    m_size--;
    if (m_delete_content)
      delete m_cursor->content;
    struct list_el *el_ptr = m_cursor;
    if (m_cursor == m_front)
    {
      m_front = m_cursor->next;
      if (m_front)
        m_front->prev = NULL;
      if (m_size == 0)
        m_back = NULL;
      delete el_ptr;
      return list_ok;
    }
    if (m_cursor == m_back)
    {
      m_back = m_cursor->prev;
      if (m_back)
        m_back->next = NULL;
      if (m_size == 0)
        m_front = NULL;
      delete el_ptr;
      return list_ok;
    }
    (m_cursor->prev)->next = m_cursor->next;
    (m_cursor->next)->prev = m_cursor->prev;
    delete el_ptr;
  }
  return list_ok;
}

template <class T>
List_err List<T>::pop_front()
{
  struct list_el *el_ptr = m_front;
  if (m_front)
  {
    m_size--;
    m_front = m_front->next;
    if (m_front)
      m_front->prev = NULL;
    if (m_delete_content)
      delete el_ptr->content;
    delete el_ptr;
    if (m_size == 0) // the list is now empty
      m_back = NULL;
    return list_ok;
  }
  else
  {
    return list_empty;
  }
}

template <class T>
List_err List<T>::pop_back()
{
  struct list_el *el_ptr = m_back;
  if (m_back)
  {
    m_size--;
    m_back = m_back->prev;
    if (m_back)
      m_back->next = NULL;
    if (m_delete_content)
      delete el_ptr->content;
    delete el_ptr;
    if (m_size == 0) // the list is now empty
      m_front = NULL;
    return list_ok;
  }
  else
  {
    return list_empty;
  }
}

#endif