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

static
gint compare_int(gconstpointer p1, gconstpointer p2) {
	int i1 = GPOINTER_TO_INT(p1);
	int i2 = GPOINTER_TO_INT(p2);
	//printf("%d %d\n", i1, i2);
	return i1 == i2 ? 0 : i1 > i2 ? 1 : -1;
}


static
gboolean traverse(gpointer key, gpointer value, gpointer data) {
	const char *sval = (const char *)value;
	printf("%s\n", sval);
	return FALSE;
}

/*
static
gint find_last(gconstpointer p, gpointer user_data) {
	return 1;
}
*/

static inline gpointer node_val(GTreeNode *node) {
	return node ? node->value : NULL;
}

static inline char *node_key(GTreeNode *node, char buf[11]) {
	return node ? (sprintf(buf, "%d", GPOINTER_TO_INT(node->key)),buf) : "NULL";
}

typedef enum {
	find_exact, find_floor, find_ceil, find_lower, find_upper
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
      if (cmp == 0)
        return node;
      else if (cmp < 0)
        {
          // искомый ключ меньше текущей ноды
          
          if (mode == find_floor && last_lesser_node) {
            return last_lesser_node;
          }
          if (!node->left_child) {
            if (mode == find_ceil) {
              return node;
            }
            return NULL;
          }

          last_greater_node = node;
          node = node->left;
        }
      else
        {
          if (mode == find_ceil && last_greater_node) {
            return last_greater_node;
          }
          if (!node->right_child) {
            if (mode == find_floor) {
              return node;
            }
            return NULL;
          }

          last_lesser_node = node;
          node = node->right;
        }
    }
}

static inline GTreeNode *_floorKey(GTree *tree, int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_floor);
}
#define floorKey(key) _floorKey(tree, key)

static inline GTreeNode *_lowerKey(GTree *tree, int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_lower);
}
#define lowerKey(key) _lowerKey(tree, key)


static inline GTreeNode *_ceilingKey(GTree *tree, int key) {
	return g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_ceil);
}
#define ceilingKey(key) _ceilingKey(tree, key)

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

#define exp_root 8
#define exp_root_left 0
#define exp_root_right 10

int main(int argc, char *argv[]) {
	GTreeNode *root_node;
	int root_key;
	//int key;
	// GTreeNode *node;
	GTree *tree = g_tree_new(compare_int);
	g_tree_insert(tree, GINT_TO_POINTER(10), "ten");
	g_tree_insert(tree, GINT_TO_POINTER(20), "twenty");
	g_tree_insert(tree, GINT_TO_POINTER(-99), "minus ninety-nine");
	g_tree_insert(tree, GINT_TO_POINTER(8), "eight");
	g_tree_insert(tree, GINT_TO_POINTER(-1), "minus one");
	g_tree_insert(tree, GINT_TO_POINTER(0), "zero");
	g_tree_foreach(tree, traverse, NULL);
	printf("=======\n");
	// node = ;
	printf("first: %s\n", NULS(node_val(g_tree_first_node(tree))));
	printf("last: %s\n", NULS(node_val(g_tree_last_node(tree))));
	//key = 11;
	//printf("floor for %d: %s\n", key, NULS(node_val(g_tree_find_node_ex(tree, GINT_TO_POINTER(key), find_floor))));
	//printf("=======\n%s\n", NULS((const char*)g_tree_search(tree, (GCompareFunc)find_last, NULL)));

	printf("=======\n");

	// ////////////////////

	root_node = tree->root;
	assertEquals(8, root_node);
	root_key = GPOINTER_TO_INT(root_node->key);
	// System.out.println("root key: " + rootKey);


	assertNull(floorKey(-100));
	assertEquals(-99, floorKey(-99));
	assertEquals(-99, floorKey(-98));


	assertEquals(exp_root_left, floorKey(root_key-1));
	assertEquals(exp_root, floorKey(root_key));
	assertEquals(exp_root, floorKey(root_key+1));

	assertEquals(10, floorKey(19));
	assertEquals(20, floorKey(20));
	assertEquals(20, floorKey(21));

	// /////////////

	assertEquals(-99, ceilingKey(-100));
	assertEquals(-99, ceilingKey(-99));
	assertEquals(-1, ceilingKey(-98));

	assertEquals(exp_root, ceilingKey(root_key-1));
	assertEquals(exp_root, ceilingKey(root_key));
	assertEquals(exp_root_right, ceilingKey(root_key+1));

	assertEquals(20, ceilingKey(19));
	assertEquals(20, ceilingKey(20));
	assertNull(ceilingKey(21));

#if 0
#endif

	// /////////////

	assertNull(lowerKey(-100));
	assertNull(lowerKey(-99));
	assertEquals(-99, lowerKey(-98));
#if 0

	assertEquals(exp_root_left, lowerKey(root_key-1));
	assertEquals(exp_root_left, lowerKey(root_key));
	assertEquals(exp_root, lowerKey(root_key+1));

	assertEquals(10, lowerKey(19));
	assertEquals(10, lowerKey(20));
	assertEquals(20, lowerKey(21));

	// /////////////

	assertEquals(-99, higherKey(-100));
	assertEquals(-1, higherKey(-99));
	assertEquals(-1, higherKey(-98));

	assertEquals(exp_root, higherKey(root_key-1));
	assertEquals(exp_root_right, higherKey(root_key));
	assertEquals(exp_root_right, higherKey(root_key+1));

	assertEquals(20, higherKey(19));
	assertNull(higherKey(20));
	assertNull(higherKey(21));

#endif

	return 0;
}

