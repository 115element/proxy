#include <stdlib.h>
#include <stdio.h>

typedef struct node {
    int data;
    struct node *left;
    struct node *right;
} Node;

typedef struct {
    Node *root;
} Tree;


void insert(Tree *tree, int value) {
    Node *node = malloc(sizeof(Node));
    node->data = value;
    node->left = NULL;
    node->right = NULL;

    if (tree->root == NULL) {
        tree->root = node;
    } else {
        Node *temp = tree->root;
        while (temp != NULL) {
            if (value < temp->data) {
                if (temp->left == NULL) {
                    temp->left = node;
                    return;
                } else {
                    temp = temp->left;
                }
            } else {
                if (temp->right == NULL) {
                    temp->right = node;
                    return;
                } else {
                    temp = temp->right;
                }
            }
        }
    }
}


void preorder(Node *node) {
    if (node != NULL) {
        printf("%d\n", node->data);
        preorder(node->left);
        preorder(node->right);
    }
}

void inorder(Node *node) {
    if (node != NULL) {
        inorder(node->left);
        printf("%d\n", node->data);
        inorder(node->right);
    }
}

void postorder(Node *node) {
    if (node != NULL) {
        postorder(node->left);
        postorder(node->right);
        printf("%d\n", node->data);
    }
}


int get_height(Node *node) {
    if (node == NULL) {
        return 0;
    } else {
        int left_h = get_height(node->left);
        int right_h = get_height(node->right);
        int max = left_h;
        if (right_h > max) {
            max = right_h;
        }
        return max + 1;
    }
}

int get_maximum(Node *node) {
    if (node == NULL) {
        return -1;
    } else {
        int m1 = get_maximum(node->left);
        int m2 = get_maximum(node->right);
        int m3 = node->data;
        int max = m1;
        if (m2 > max) {
            max = m2;
        }
        if (m3 > max) {
            max = m3;
        }
        return max;
    }
}


int main() {

    int arr[7] = {6, 3, 8, 2, 5, 1, 7};
    Tree tree;
    tree.root = NULL;

    int i;
    for (i = 0; i < 7; i++) {
        insert(&tree, arr[i]);
    }

    preorder(tree.root);

    int h = get_height(tree.root);
    printf("h = %d", h);

    int max = get_maximum(tree.root);
    printf("max = %d",max);
    return 0;
}


//删除二叉搜索树中的节点有以下五种情况：
//
//第一种情况：没找到删除的节点，遍历到空节点直接返回了
// 找到删除的节点
//第二种情况：左右孩子都为空（叶子节点），直接删除节点， 返回NULL为根节点
//第三种情况：删除节点的左孩子为空，右孩子不为空，删除节点，右孩子补位，返回右孩子为根节点
//第四种情况：删除节点的右孩子为空，左孩子不为空，删除节点，左孩子补位，返回左孩子为根节点
//第五种情况：左右孩子节点都不为空，则将删除节点的左子树头结点（左孩子）放到删除节点的右子树的最左面节点的左孩子上，返回删除节点右孩子为新的根节点。
//-----------------------------------
