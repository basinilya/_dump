#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

static inline const char *NULS(const char *s) {
	return s ? s : "NULL";
}

/* opaque glib structures */
typedef struct _GTreeNode GTreeNode;

struct _GTree
{
  GTreeNode        *root;
  GCompareDataFunc  key_compare;
  GDestroyNotify    key_destroy_func;
  GDestroyNotify    value_destroy_func;
  gpointer          key_compare_data;
  guint             nnodes;
  gint              ref_count;
};

struct _GTreeNode
{
  gpointer   key;         /* key for this node */
  gpointer   value;       /* value stored at this node */
  GTreeNode *left;        /* left subtree */
  GTreeNode *right;       /* right subtree */
  gint8      balance;     /* height (right) - height (left) */
  guint8     left_child;
  guint8     right_child;
};

static inline GTreeNode *
g_tree_first_node(GTree *tree)
{
  GTreeNode *tmp;

  if (!tree->root)
    return NULL;

  tmp = tree->root;

  while (tmp->left_child)
    tmp = tmp->left;

  return tmp;
}

static inline GTreeNode *
g_tree_last_node(GTree *tree)
{
  GTreeNode *tmp;

  if (!tree->root)
    return NULL;

  tmp = tree->root;

  while (tmp->right_child)
    tmp = tmp->right;

  return tmp;
}

typedef enum {
	find_exact = 0,
	find_floor = 0x2,
	find_ceil  = 0x20,
	find_lower = find_floor + 1,
	find_higher = find_ceil + 1
} find_mode;

static GTreeNode *
g_tree_find_node_ex (GTree        *tree,
                  gconstpointer key,
                  find_mode mode
                  )
{
  GTreeNode *node;
  gint cmp;
  GTreeNode *last_lesser_node = NULL;
  GTreeNode *last_greater_node = NULL;

  node = tree->root;
  if (!node)
    return NULL;

  while (1)
    {
      cmp = tree->key_compare (key, node->key, tree->key_compare_data);
      if (cmp == 0) {
        if (mode == find_lower) {
          cmp = -1;
        } else if (mode == find_higher) {
          cmp = 1;
        } else {
          return node;
        }
      }

      if (cmp < 0)
        {
          if (!node->left_child) {
            if ( (mode & find_floor) ) {
              return last_lesser_node; /* can be null */
            }
            if ( (mode & find_ceil) ) {
              return node;
            }
            return NULL;
          }

          last_greater_node = node;
          node = node->left;
        }
      else
        {
          if (!node->right_child) {
            if ( (mode & find_ceil) ) {
              return last_greater_node; /* can be null */
            }
            if ( (mode & find_floor) ) {
              return node;
            }
            return NULL;
          }

          last_lesser_node = node;
          node = node->right;
        }
    }
}





#ifdef GTREEEX_DEBUG

static
gint compare_int(gconstpointer p1, gconstpointer p2) {
	int i1 = GPOINTER_TO_INT(p1);
	int i2 = GPOINTER_TO_INT(p2);
	/* printf("%d %d\n", i1, i2); */
	return i1 == i2 ? 0 : i1 > i2 ? 1 : -1;
}


static inline char *node_key(GTreeNode *node, char buf[11]) {
	return node ? (sprintf(buf, "%d", GPOINTER_TO_INT(node->key)),buf) : "NULL";
}

static GTree *tree;

static inline void put(int key, int val) {
	g_tree_insert(tree, GINT_TO_POINTER(key), GINT_TO_POINTER(key));
}

static inline GTreeNode *floorKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_floor);
}

static inline GTreeNode *lowerKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_lower);
}

static inline GTreeNode *higherKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_higher);
}

static inline GTreeNode *ceilingKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_ceil);
}

static inline void _assertEquals(int line, int expected, GTreeNode *actual_node) {
	char buf[11];
	if (actual_node && GINT_TO_POINTER(expected) == actual_node->key) return;
	printf(":%d expected: %d actual: %s\n", line, expected, node_key(actual_node, buf));
	exit(1);
}

static inline void _assertNull(int line, GTreeNode *actual_node) {
	char buf[11];
	if (!actual_node) return;
	printf(":%d expected: NULL actual: %s\n", line, node_key(actual_node, buf));
	exit(1);
}
#define assertNull(actual_node) _assertNull(__LINE__, actual_node)
#define assertEquals(expected, actual_node) _assertEquals(__LINE__, expected, actual_node)

int main(int argc, char *argv[]) {

	tree = g_tree_new(compare_int);

	for (int i = -9; i <= 9; i += 4) {
		put(i, i);
	}

assertNull(floorKey(-10));
assertEquals(-9, ceilingKey(-10));
assertNull(lowerKey(-10));
assertEquals(-9, higherKey(-10));
assertEquals(-9, floorKey(-9));
assertEquals(-9, ceilingKey(-9));
assertNull(lowerKey(-9));
assertEquals(-5, higherKey(-9));
assertEquals(-9, floorKey(-8));
assertEquals(-5, ceilingKey(-8));
assertEquals(-9, lowerKey(-8));
assertEquals(-5, higherKey(-8));
assertEquals(-9, floorKey(-7));
assertEquals(-5, ceilingKey(-7));
assertEquals(-9, lowerKey(-7));
assertEquals(-5, higherKey(-7));
assertEquals(-9, floorKey(-6));
assertEquals(-5, ceilingKey(-6));
assertEquals(-9, lowerKey(-6));
assertEquals(-5, higherKey(-6));
assertEquals(-5, floorKey(-5));
assertEquals(-5, ceilingKey(-5));
assertEquals(-9, lowerKey(-5));
assertEquals(-1, higherKey(-5));
assertEquals(-5, floorKey(-4));
assertEquals(-1, ceilingKey(-4));
assertEquals(-5, lowerKey(-4));
assertEquals(-1, higherKey(-4));
assertEquals(-5, floorKey(-3));
assertEquals(-1, ceilingKey(-3));
assertEquals(-5, lowerKey(-3));
assertEquals(-1, higherKey(-3));
assertEquals(-5, floorKey(-2));
assertEquals(-1, ceilingKey(-2));
assertEquals(-5, lowerKey(-2));
assertEquals(-1, higherKey(-2));
assertEquals(-1, floorKey(-1));
assertEquals(-1, ceilingKey(-1));
assertEquals(-5, lowerKey(-1));
assertEquals(3, higherKey(-1));
assertEquals(-1, floorKey(0));
assertEquals(3, ceilingKey(0));
assertEquals(-1, lowerKey(0));
assertEquals(3, higherKey(0));
assertEquals(-1, floorKey(1));
assertEquals(3, ceilingKey(1));
assertEquals(-1, lowerKey(1));
assertEquals(3, higherKey(1));
assertEquals(-1, floorKey(2));
assertEquals(3, ceilingKey(2));
assertEquals(-1, lowerKey(2));
assertEquals(3, higherKey(2));
assertEquals(3, floorKey(3));
assertEquals(3, ceilingKey(3));
assertEquals(-1, lowerKey(3));
assertEquals(7, higherKey(3));
assertEquals(3, floorKey(4));
assertEquals(7, ceilingKey(4));
assertEquals(3, lowerKey(4));
assertEquals(7, higherKey(4));
assertEquals(3, floorKey(5));
assertEquals(7, ceilingKey(5));
assertEquals(3, lowerKey(5));
assertEquals(7, higherKey(5));
assertEquals(3, floorKey(6));
assertEquals(7, ceilingKey(6));
assertEquals(3, lowerKey(6));
assertEquals(7, higherKey(6));
assertEquals(7, floorKey(7));
assertEquals(7, ceilingKey(7));
assertEquals(3, lowerKey(7));
assertNull(higherKey(7));
assertEquals(7, floorKey(8));
assertNull(ceilingKey(8));
assertEquals(7, lowerKey(8));
assertNull(higherKey(8));

	printf("PASSED\n");

	return 0;
}

#endif /* GTREEEX_DEBUG */
