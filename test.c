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
	FIND_EXACT = 0,
	FIND_FLOOR = 0x2,
	FIND_CEIL  = 0x20,
	FIND_LOWER = (FIND_FLOOR + 1),
	FIND_HIGHER = (FIND_CEIL + 1)
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
        if (mode == FIND_LOWER) {
          cmp = -1;
        } else if (mode == FIND_HIGHER) {
          cmp = 1;
        } else {
          return node;
        }
      }

      if (cmp < 0)
        {
          if (!node->left_child) {
            if ( (mode & FIND_FLOOR) ) {
              return last_lesser_node; /* can be null */
            }
            if ( (mode & FIND_CEIL) ) {
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
            if ( (mode & FIND_CEIL) ) {
              return last_greater_node; /* can be null */
            }
            if ( (mode & FIND_FLOOR) ) {
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

static inline GTreeNode *get(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), FIND_EXACT);
}

static inline GTreeNode *floorKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), FIND_FLOOR);
}

static inline GTreeNode *lowerKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), FIND_LOWER);
}

static inline GTreeNode *higherKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), FIND_HIGHER);
}

static inline GTreeNode *ceilingKey(int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), FIND_CEIL);
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

	for (int i = -9; i <= 9; i += 3) {
		put(i, i);
	}

assertNull(get(-10));
assertNull(floorKey(-10));
assertEquals(-9, ceilingKey(-10));
assertNull(lowerKey(-10));
assertEquals(-9, higherKey(-10));
assertEquals(-9, get(-9));
assertEquals(-9, floorKey(-9));
assertEquals(-9, ceilingKey(-9));
assertNull(lowerKey(-9));
assertEquals(-6, higherKey(-9));
assertNull(get(-8));
assertEquals(-9, floorKey(-8));
assertEquals(-6, ceilingKey(-8));
assertEquals(-9, lowerKey(-8));
assertEquals(-6, higherKey(-8));
assertNull(get(-7));
assertEquals(-9, floorKey(-7));
assertEquals(-6, ceilingKey(-7));
assertEquals(-9, lowerKey(-7));
assertEquals(-6, higherKey(-7));
assertEquals(-6, get(-6));
assertEquals(-6, floorKey(-6));
assertEquals(-6, ceilingKey(-6));
assertEquals(-9, lowerKey(-6));
assertEquals(-3, higherKey(-6));
assertNull(get(-5));
assertEquals(-6, floorKey(-5));
assertEquals(-3, ceilingKey(-5));
assertEquals(-6, lowerKey(-5));
assertEquals(-3, higherKey(-5));
assertNull(get(-4));
assertEquals(-6, floorKey(-4));
assertEquals(-3, ceilingKey(-4));
assertEquals(-6, lowerKey(-4));
assertEquals(-3, higherKey(-4));
assertEquals(-3, get(-3));
assertEquals(-3, floorKey(-3));
assertEquals(-3, ceilingKey(-3));
assertEquals(-6, lowerKey(-3));
assertEquals(0, higherKey(-3));
assertNull(get(-2));
assertEquals(-3, floorKey(-2));
assertEquals(0, ceilingKey(-2));
assertEquals(-3, lowerKey(-2));
assertEquals(0, higherKey(-2));
assertNull(get(-1));
assertEquals(-3, floorKey(-1));
assertEquals(0, ceilingKey(-1));
assertEquals(-3, lowerKey(-1));
assertEquals(0, higherKey(-1));
assertEquals(0, get(0));
assertEquals(0, floorKey(0));
assertEquals(0, ceilingKey(0));
assertEquals(-3, lowerKey(0));
assertEquals(3, higherKey(0));
assertNull(get(1));
assertEquals(0, floorKey(1));
assertEquals(3, ceilingKey(1));
assertEquals(0, lowerKey(1));
assertEquals(3, higherKey(1));
assertNull(get(2));
assertEquals(0, floorKey(2));
assertEquals(3, ceilingKey(2));
assertEquals(0, lowerKey(2));
assertEquals(3, higherKey(2));
assertEquals(3, get(3));
assertEquals(3, floorKey(3));
assertEquals(3, ceilingKey(3));
assertEquals(0, lowerKey(3));
assertEquals(6, higherKey(3));
assertNull(get(4));
assertEquals(3, floorKey(4));
assertEquals(6, ceilingKey(4));
assertEquals(3, lowerKey(4));
assertEquals(6, higherKey(4));
assertNull(get(5));
assertEquals(3, floorKey(5));
assertEquals(6, ceilingKey(5));
assertEquals(3, lowerKey(5));
assertEquals(6, higherKey(5));
assertEquals(6, get(6));
assertEquals(6, floorKey(6));
assertEquals(6, ceilingKey(6));
assertEquals(3, lowerKey(6));
assertEquals(9, higherKey(6));
assertNull(get(7));
assertEquals(6, floorKey(7));
assertEquals(9, ceilingKey(7));
assertEquals(6, lowerKey(7));
assertEquals(9, higherKey(7));
assertNull(get(8));
assertEquals(6, floorKey(8));
assertEquals(9, ceilingKey(8));
assertEquals(6, lowerKey(8));
assertEquals(9, higherKey(8));
assertEquals(9, get(9));
assertEquals(9, floorKey(9));
assertEquals(9, ceilingKey(9));
assertEquals(6, lowerKey(9));
assertNull(higherKey(9));
assertNull(get(10));
assertEquals(9, floorKey(10));
assertNull(ceilingKey(10));
assertEquals(9, lowerKey(10));
assertNull(higherKey(10));


	printf("PASSED\n");

	return 0;
}

#endif /* GTREEEX_DEBUG */
