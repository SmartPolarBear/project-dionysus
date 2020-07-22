#pragma once
#include "system/types.h"

#include <algorithm>

using std::max;

template<typename TKey, typename TData>
class AVLTree
{
 private:
	struct node
	{
		TKey key;
		TData data;

		size_t height;
		node* left, * right;
	};

	inline int subtree_height(node* n)
	{
		if (n == nullptr)
		{
			return 0;
		}
		else
		{
			return n->height;
		}
	}

	inline node* new_node(TKey key, TData data)
	{
		node* n = new node;

		n->key = key;
		n->data = data;

		n->left = nullptr;
		n->right = nullptr;
		n->height = 1;

		return n;
	}

	inline node* right_rotate(node* y)
	{
		node* x = y->left;
		node* t2 = x->right;

		x->right = y;
		y->left = t2;

		y->height = max(subtree_height(y->left), subtree_height(y->right)) + 1;
		x->height = max(subtree_height(x->left), subtree_height(x->right)) + 1;

		return x; // new root
	}

	inline node* left_rotate(node* x)
	{
		node* y = x->right;
		node* t2 = y->left;

		y->left = x;
		x->right = t2;

		x->height = max(subtree_height(x->left), subtree_height(x->right)) + 1;
		y->height = max(subtree_height(y->left), subtree_height(y->right)) + 1;

		return y; // new root
	}

	inline int balance_factor(node* n)
	{
		if (n == nullptr)
		{
			return 0;
		}
		return subtree_height(n->left) - subtree_height(n->right);
	}


	inline node* min_node(node* root)
	{
		node* current = root;
		while (current->left)
			current = current->left;

		return current;
	}

	node* insert(node* root, TKey key, TData data)
	{
		if (root == nullptr)
		{
			m_size++;
			return new_node(key, data);
		}

		if (key < root->key)
		{
			root->left = insert(root->left, key, data);
		}
		else if (key > root->key)
		{
			root->right = insert(root->right, key, data);
		}
		else
		{
			// We can't add equal values
			// TODO: exception
			return root;
		}

		// update height
		root->height = max(subtree_height(root->left), subtree_height(root->right)) + 1;

		// balance the tree
		auto factor = balance_factor(root);

		// left left
		if (factor > 1 && root->left && key < root->left->key)
		{
			return right_rotate(root);
		}

		// right right
		if (factor<-1 && root->right && key>root->right->key)
		{
			return left_rotate(root);
		}

		// left right
		if (factor > 1 && root->left && key > root->left->key)
		{
			root->left = left_rotate(root->left);
			return right_rotate(root);
		}

		// right left
		if (factor < -1 && root->right && key < root->right->key)
		{
			root->right = right_rotate(root->right);
			return left_rotate(root);
		}

		// no need to re-balance
		return root;

	}

	node* remove(node* root, TKey key)
	{
		if (root == nullptr)
		{
			return root;
		}

		if (key < root->key)
		{
			root->left = remove(root->left, key);
		}
		else if (key > root->key)
		{
			root->right = remove(root->right, key);
		}
		else
		{
			if (root->left == nullptr || root->right == nullptr)
			{
				node* victim = root->left ? root->left : root->right;
				if (victim == nullptr)
				{
					victim = root;
					root = nullptr;
				}
				else
				{
					*root = *victim;
				}

				m_size--;
				delete victim;
			}
			else
			{
				node* victim = min_node(root->right);

				root->key = victim->key;
				root->right = remove(root->right, victim->key);
			}
		}

		if (root == nullptr)
		{
			return root;
		}

		root->height = max(subtree_height(root->left), subtree_height(root->right)) + 1;

		auto factor = balance_factor(root);

		// Left Left Case
		if (factor > 1 && balance_factor(root->left) >= 0)
		{
			return right_rotate(root);
		}

		// Left Right Case
		if (factor > 1 && balance_factor(root->left) < 0)
		{
			root->left = left_rotate(root->left);
			return right_rotate(root);
		}

		// Right Right Case
		if (factor < -1 && balance_factor(root->right) <= 0)
		{
			return left_rotate(root);
		}

		// Right Left Case
		if (factor < -1 && balance_factor(root->right) > 0)
		{
			root->right = right_rotate(root->right);
			return left_rotate(root);
		}

		return root;
	}

	node* find(node* root, TKey key)
	{
		node* iter = root;
		while (iter)
		{
			if (key > iter->key)
			{
				iter = iter->right;
			}
			else if (key < iter->key)
			{
				iter = iter->left;
			}
			else if (key == iter->key)
			{
				return iter;
			}
		}

		return nullptr;
	}

	void clear(node* n)
	{
		if (n->left != nullptr)
		{
			clear(n->left);
		}

		if (n->right != nullptr)
		{
			clear(n->right);
		}

		delete n;
		n = nullptr;
	}

 private:
	node* root;
	size_t m_size;

 public:
	AVLTree() :root(nullptr), m_size(0)
	{

	}

	~AVLTree()
	{
		clear();
	}


	void insert(TKey key, TData data)
	{
		root = insert(root, key, data);
	}

	void clear()
	{
		clear(root);
		root = nullptr;
		m_size = 0;
	}

	void remove(TKey key)
	{
		root = remove(root, key);
	}

	TData& find(TKey key)
	{
		auto node = find(root, key);
		if (node == nullptr)
		{
			insert(key, {});
			node = find(root, key);
		}

		return node->data;
	}

	TData& operator[](TKey key)
	{
		return find(key);
	}

	size_t size()
	{
		return m_size;
	}
};