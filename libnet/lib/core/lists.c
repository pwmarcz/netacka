/* Routines to manipulate lists of drivers */

#include <stdlib.h>
#include "libnet.h"
#include "internal.h"

list_t net_driverlist_create (void)
{
  list_t list = malloc (sizeof (*list));
  if (list)
    net_driverlist_clear (list);
  return list;
}

void net_driverlist_destroy (list_t list)
{
  free (list);
}

int net_driverlist_clear (list_t list)
{
  *list = 0;
  return 1;
}

int net_driverlist_add (list_t list, int driver)
{
  *list |= (1 << driver);
  return 1;
}

int net_driverlist_remove (list_t list, int driver)
{
  *list &= ~(1 << driver);
  return 1;
}

int net_driverlist_test (list_t list, int driver)
{
  return !!(*list & (1 << driver));
}

int net_driverlist_foreach (list_t list, int (*func)(int driver, void *dat), void *dat)
{
  unsigned int j = 0, k = *list;
  while (k) {
    if ((k & 1) && func (j, dat))
      return 0;
    k >>= 1;
    j++;
  }
  return 1;
}

static int count_helper (int driver, void *dat)
  { (*(int *)dat)++; return 0; }
int net_driverlist_count (list_t list)
  { int count = 0; net_driverlist_foreach (list, count_helper, &count); return count; }

static int add_helper (int driver, void *dat)
  { return !net_driverlist_add (dat, driver); }
static int remove_helper (int driver, void *dat)
  { return !net_driverlist_remove (dat, driver); }

int net_driverlist_add_list (list_t list1, list_t list2)
  { list_t some_list = list1; return net_driverlist_foreach (list2, add_helper, some_list); }

int net_driverlist_remove_list (list_t list1, list_t list2)
  { list_t some_list = list1; return net_driverlist_foreach (list2, remove_helper, some_list); }

